package com.android.rs.refocus;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Environment;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.nio.ByteBuffer;

/**
 * Created by xinyiwang on 6/30/15.
 */
public class ImageCompare {
    private static byte[] loadImageByteArray(String file_path) {
        Bitmap bitmap = BitmapFactory.decodeFile(file_path);
        ByteArrayOutputStream stream = new ByteArrayOutputStream();
        bitmap.compress(Bitmap.CompressFormat.PNG, 100, stream);
        return stream.toByteArray();
    }

    public static boolean compareImage(String file1, String file2) {
        byte[] first = loadImageByteArray(file1);
        byte[] second = loadImageByteArray(file2);

        for (int i = 0; i < first.length; i++) {
            int v1 = 0xFF & first[i];
            int v2 = 0xFF & second[i];
            int error = Math.abs(v1 - v2);
            if (error > 2) {
                return false;
            }
        }
        return true;
    }

    private static byte[] loadBitmapByteArray(Bitmap bitmap) {
        int bytes = bitmap.getByteCount();
        ByteBuffer buffer = ByteBuffer.allocate(bytes);
        bitmap.copyPixelsToBuffer(buffer);
        byte[] array = buffer.array();
        return array;
    }

    public static class CompareValue {
      float aveDiff = 0;
      float diffPercent = 0f;
    }

    public static void compareBitmap(Bitmap bitmap1, Bitmap bitmap2, CompareValue result) {
        byte[] first = loadBitmapByteArray(bitmap1);
        byte[] second = loadBitmapByteArray(bitmap2);

        int loopCount = first.length > second.length ? second.length : first.length;

        int diffCount = 0;
        int diffSum = 0;
        for (int i = 0; i < loopCount; i++) {
          int v1 = 0xFF & first[i];
          int v2 = 0xFF & second[i];
          int error = Math.abs(v1 - v2);
          if (error > 0) {
            diffCount++;
            //if (error > result.maxDiff) {
              //result.maxDiff = error;
            //}
            diffSum += error;
          }
        }
        result.diffPercent = ((float)diffCount)/first.length;
        result.aveDiff = ((float)diffSum)/first.length;
    }

    public static void compareIntermediate(String folder1, String folder2) {
        File folder = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES);
        String folder_path = folder.getAbsolutePath();
        //String folder_path = "/storage/self/primary/";
        String file_path_1_base = folder_path + "/" + folder1;
        String file_path_2_base = folder_path + "/" + folder2;
        File dir1 = new File(file_path_1_base);

        for ( File imgFile : dir1.listFiles()) {
           String file_path_2 = file_path_2_base + "/" + imgFile.getName();
            String file_path_1 = file_path_1_base + "/" + imgFile.getName();
            System.out.println(file_path_1);
            System.out.println(file_path_2);
            boolean same = compareImage(file_path_1, file_path_2);
            if (same) {
                Log.d("imageCompare:", imgFile.getName() + " is the same!");
            } else {
                Log.d("imageCompare:", imgFile.getName() + " is different!");
            }
        }
    }

    public static void printWrongIndex(String folder1, String folder2, String file, Context mContext) {
        File folder = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES);
        String folder_path = folder.getAbsolutePath();
        //String folder_path = "/storage/self/primary/";
        String file_path_1 = folder_path + "/" + folder1 + "/" + file;
        String file_path_2 = folder_path + "/" + folder2 + "/" + file;



        Bitmap bitmap1 = BitmapFactory.decodeFile(file_path_1);
        ByteArrayOutputStream stream1 = new ByteArrayOutputStream();
        bitmap1.compress(Bitmap.CompressFormat.PNG, 100, stream1);
        byte[] first =  stream1.toByteArray();

        Bitmap bitmap2 = BitmapFactory.decodeFile(file_path_1);
        ByteArrayOutputStream stream2 = new ByteArrayOutputStream();
        bitmap2.compress(Bitmap.CompressFormat.PNG, 100, stream2);
        byte[] second = stream2.toByteArray();

        int width = bitmap1.getWidth();
        int height = bitmap1.getHeight();

        byte[] difference = new byte[first.length];
        //System.out.println("Total pixel: " + width * height);
        //System.out.println("intdifference length: " + intdifference.length);
        //System.out.println("byte array length" + first.length);

        for (int i = 0; i < first.length; i++) {
            int v1 = 0xFF & first[i];
            int v2 = 0xFF & second[i];
            int error = Math.abs(v1 - v2);
            //if (error > 2 ) {
            //    intdifference[i/4] = 0;
            //} else {
            //    intdifference[i/4] = 255;
            //}
            difference[i] = (byte)(first[i] - second[i]);
        }

        //Bitmap differenceBitmap = Bitmap.createBitmap(difference, width, height, Bitmap.Config.ARGB_8888);

        Bitmap differenceBitmap = BitmapFactory.decodeByteArray(difference,0, difference.length);
        MediaStoreSaver.savePNG(differenceBitmap, "difference", "updateSharp1Difference.png", mContext);
        System.out.println("difference image saved!");
    }
}
