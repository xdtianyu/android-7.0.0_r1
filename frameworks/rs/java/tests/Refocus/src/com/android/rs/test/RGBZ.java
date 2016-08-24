package com.android.rs.refocus;

import android.content.ContentResolver;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.net.Uri;
import android.util.Log;

import java.io.FileNotFoundException;
import java.io.IOException;

/**
 * An RGBZ image, where Z stands for depth, i.e. a color+depth image.
 * The RGBZ always has a preview image, which represents the latest rendering of the RGBZ.
 * The preview is encoded as the normal jpeg content for client compatibility,
 * while the color channel and depth channels are encoded as XMP data.
 * The class supports lazy initialization where the XMP meta data is loaded only when first
 * accessed.
 *
 * @author chernand@google.com (Carlos Hernandez)
 */
public class RGBZ {
  public static final String TAG = "RGBZ";

  private Bitmap bitmap;
  private Bitmap preview;
  private Bitmap depthBitmap;
  private DepthTransform depthTransform;
  private DepthImage depthImage;

  /**
   * Creates an RGBZ from a content uri.
   *
   * @param uri The uri name of the RGBZ
   * @throws FileNotFoundException if the RGBZ could not be read
   */
  public RGBZ(Uri uri, ContentResolver contentResolver, Context context) throws IOException {
    preview = BitmapFactory.decodeStream(contentResolver.openInputStream(uri));
    if (preview == null) {
      throw new FileNotFoundException(uri.toString());
    }
    this.depthImage = new DepthImage(context, uri);
    this.depthBitmap = depthImage.getDepthBitmap();
    this.bitmap = setAlphaChannel(preview, this.depthBitmap);
    this.depthTransform = depthImage.getDepthTransform();
  }

  /**
   * @return Whether the RGBZ has a depth channel
   */
  public boolean hasDepthmap() {
    return depthTransform != null;
  }

  /**
   * @return The color+depth {@code Bitmap}
   */
  public Bitmap getBitmap() {
    return bitmap;
  }

  /**
   * @return The depthmap component of this RGBZ
   */
  public DepthTransform getDepthTransform() {
    return depthTransform;
  }

  public double getFocusDepth() {
    return this.depthImage.getFocalDistance();
  }

  public double getDepthOfField() {
    return this.depthImage.getDepthOfField();
  }

  public double getBlurInfinity() {
    return this.depthImage.getBlurAtInfinity();
  }

  /**
   * @return the width of this {@code RGBZ}
   */
  public int getWidth() {
    return bitmap.getWidth();
  }

  /**
   * @return the height of this {@code RGBZ}
   */
  public int getHeight() {
    return bitmap.getHeight();
  }

  /**
   * @return the depth value of the given pixel
   */

  public float getDepth(int x, int y) {
    if (!hasDepthmap()) {
      return 0.0f;
    }
    if (x < 0 || x > depthBitmap.getWidth() ||
            y < 0 || y > depthBitmap.getHeight()) {
      Log.e("RGBZ getDepth", "index out of bound");
      return 0;
    }
    return getDepthTransform().reconstruct(Color.blue(depthBitmap.getPixel(x, y)));
  }

  /**
   * Sets the depthmap as the alpha channel of the {@code Bitmap}.
   */
  public Bitmap setAlphaChannel(Bitmap bitmap, Bitmap depthBitmap) {
    if (bitmap == null) {
      return bitmap;
    }
    Bitmap result = bitmap.copy(Bitmap.Config.ARGB_8888, true);
    // set the alpha channel of depthBitmap to alpha of bitmap
    result = setAlphaChannelFromBitmap(depthBitmap, bitmap, result);
    return result;
  }

  private Bitmap setAlphaChannelFromBitmap(Bitmap depth, Bitmap orig, Bitmap dest) {
    int w = orig.getWidth();
    int h = orig.getHeight();
    int[] orig_data = new int[w*h];
    int[] depth_data = new int[w*h];

    orig.getPixels(orig_data, 0, w, 0, 0, w, h);
    depth.getPixels(depth_data, 0, w, 0, 0, w, h);
    for (int i = 0; i < orig_data.length; i++) {
      int v = orig_data[i] & 0x00FFFFFF;
      int temp = (depth_data[i] & 0x000000FF) << 24;
      v = v | temp;
      orig_data[i] = v;
    }
    dest.setPixels(orig_data, 0, w, 0, 0, w, h);
    return dest;
  }
}

