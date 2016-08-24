package com.android.rs.refocus;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Environment;
import android.os.Bundle;
import android.support.v8.renderscript.RenderScript;
import android.util.Log;
import android.widget.ImageView;
import android.widget.TextView;

import java.io.File;
import java.io.IOException;

public class MainActivity extends Activity {
    private static final int RS_API = 19;
    private static final String TAG = "MainActivity";

    ImageView mImgView;

    ImageView mNewImgView;
    TextView mCompareTextView;
    TextView mPointerLabelTextView;
    TextView mAllocLabelTextView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mImgView = (ImageView) findViewById(R.id.image_view);
        mNewImgView = (ImageView) findViewById(R.id.image_view_new);
        mCompareTextView = (TextView) findViewById(R.id.compareTextView);
        mPointerLabelTextView = (TextView) findViewById(R.id.orignialImageLabel);
        mAllocLabelTextView = (TextView) findViewById(R.id.newImageLabel);

        Intent intent = getIntent();
        if (intent != null) {

            String s = intent.getType();
            if (s != null && s.indexOf("image/") != -1) {
                Uri data = (Uri) intent.getParcelableExtra(Intent.EXTRA_STREAM);
                if (data != null) {

                    try {
                      RenderScript renderScript = RenderScript.create(getApplicationContext(), RS_API);
                      renderScript.setPriority(RenderScript.Priority.NORMAL);

                      // Get input uri to RGBZ
                      RGBZ current_rgbz = new RGBZ(data, getContentResolver(), this);
                      DepthOfFieldOptions current_depth_options = new DepthOfFieldOptions(current_rgbz);

                      // Set image focus settings
                      current_depth_options.setFocusPoint(0.7f, 0.5f);
                      current_depth_options.setBokeh(2f);

                      RsTaskParams rsTaskParam = new RsTaskParams(renderScript, current_depth_options);
                      new RsAsyncTaskRunner().execute(rsTaskParam);
                      return;
                    } catch (IOException e) {
                        e.printStackTrace();
                    }

                }
            }
        }


        try {
            //Uri data = getLocalRef();
            Uri data = getResourceRef();
            if (data == null) {
                return;
            }

            RenderScript renderScript = RenderScript.create(getApplicationContext(), RS_API);
            renderScript.setPriority(RenderScript.Priority.NORMAL);

            // Get input uri to RGBZ
            RGBZ current_rgbz = new RGBZ(data, getContentResolver(), this);
            DepthOfFieldOptions current_depth_options = new DepthOfFieldOptions(current_rgbz);

            // Set image focus settings
            current_depth_options.setFocusPoint(0.7f, 0.5f);
            current_depth_options.setBokeh(2f);

            RsTaskParams rsTaskParam = new RsTaskParams(renderScript, current_depth_options);
            new RsAsyncTaskRunner().execute(rsTaskParam);
            return;
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private static class RsTaskParams {
      RenderScript mRenderScript;
      DepthOfFieldOptions mOptions;

      RsTaskParams(RenderScript renderScript,
                   DepthOfFieldOptions options) {
        mRenderScript = renderScript;
        mOptions = options;
      }
    }

    private class RsAsyncTaskRunner extends AsyncTask<RsTaskParams, Integer, Bitmap> {

      Bitmap outputImage;
      Bitmap outputImageNew;
      ImageCompare.CompareValue result;

      @Override
      protected Bitmap doInBackground(RsTaskParams... params) {

        publishProgress(0);

        RenderScriptTask renderScriptTask = new RenderScriptTask(params[0].mRenderScript, RenderScriptTask.script.f32);
        outputImage = renderScriptTask.applyRefocusFilter(params[0].mOptions);
        publishProgress(1);

        RenderScriptTask renderScriptTaskNew = new RenderScriptTask(params[0].mRenderScript, RenderScriptTask.script.d1new);
        outputImageNew = renderScriptTaskNew.applyRefocusFilter(params[0].mOptions);
        publishProgress(2);

        result = new ImageCompare.CompareValue();
        ImageCompare.compareBitmap(outputImage, outputImageNew, result);
        publishProgress(3);

        return outputImage;
      }

      protected  void onPostExecute(Bitmap result) {

      }

      protected void onProgressUpdate(Integer... progress) {
        switch (progress[0]){
          case 0:
              mAllocLabelTextView.setText("Global Allocation Version...");
              mPointerLabelTextView.setText("Processing...");
              mCompareTextView.setText("Image Difference");
          case 1:
              mImgView.setImageBitmap(outputImage);
              mImgView.invalidate();
              mPointerLabelTextView.setText("Pointer Result");
              mAllocLabelTextView.setText("Processing...");
            break;
          case 2:
              mNewImgView.setImageBitmap(outputImageNew);
              mNewImgView.invalidate();
              mAllocLabelTextView.setText("Global Allocation Version");
              mCompareTextView.setText("Calculating...");
            break;
          case 3:
              mCompareTextView.setText("Percentage Different: " + result.diffPercent + " Average Difference: " + result.aveDiff);
            break;
        }
      }
    }

    Uri getLocalRef() {


        File folder = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM);

        Log.v(TAG, "DIRECTORY_DOCUMENTS = " + folder.getAbsolutePath());
        ;
        File f = findJpeg(folder);
        if (f != null) {
            Log.v(TAG, "File = " + f);
            return Uri.fromFile(f);
        }
        return null;
    }

    Uri getResourceRef() {
        Context context = getApplicationContext();
        int resID = R.drawable.refocusimage;
        Uri path = Uri.parse(ContentResolver.SCHEME_ANDROID_RESOURCE + "://" +
            context.getResources().getResourcePackageName(resID) + '/' +
            context.getResources().getResourceTypeName(resID) + '/' +
            context.getResources().getResourceEntryName(resID));
        return path;
    }

    private File findJpeg(File dir) {

        File[] files = dir.listFiles();
        if (files == null) return null;
        for (int i = 0; i < files.length; i++) {
            if (files[i].isDirectory() && !files[i].getName().startsWith(".")) {
                File ret = findJpeg(files[i]);
                if (ret != null) {
                    Log.v(TAG, "returning " + ret.getAbsolutePath());
                    return ret;
                }
                continue;
            }
            if (files[i].getName().toLowerCase().endsWith(".jpg")) {
                Log.v(TAG, "returning " + files[i].getAbsolutePath());
                return files[i];

            }
        }
        return null;
    }

}
