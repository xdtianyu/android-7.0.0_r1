package com.android.rs.refocus;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;


import android.os.AsyncTask;
import android.support.v8.renderscript.Allocation;
import android.support.v8.renderscript.RenderScript;

import java.io.IOException;
import java.io.InputStream;

/**
 * Created by hoford on 5/15/15.
 */
public class DepthImage {
    private final String mFormat;
    private final double mFar;
    private final double mNear;
    private final Bitmap mDepthBitmap;
    private final double mBlurAtInfinity;
    private final double mFocalDistance;
    private final double mDepthOfFiled;
    private final double mFocalPointX;
    private final double mFocalPointY;
    private final DepthTransform mDepthTransform;
    public DepthImage(Context context, Uri data) throws IOException {
        InputStream input = context.getContentResolver().openInputStream(data);
        XmpDepthDecode decode = new XmpDepthDecode(input);
        mFormat = decode.getFormat();
        mFar = decode.getFar();
        mNear = decode.getNear();
        mDepthBitmap = decode.getDepthBitmap();
        mBlurAtInfinity = decode.getBlurAtInfinity();
        mFocalDistance = decode.getFocalDistance();
        mDepthOfFiled = decode.getDepthOfField();
        mFocalPointX = decode.getFocalPointX();
        mFocalPointY = decode.getFocalPointY();
        input = context.getContentResolver().openInputStream(data);
        mDepthTransform = decode.getDepthTransform();
    }

    public Bitmap getDepthBitmap() {
        return mDepthBitmap;
    }

    public DepthTransform getDepthTransform() { return mDepthTransform; }

    public String getFormat() {
        return mFormat;
    }

    public double getFar() {
        return mFar;
    }

    public double getNear() {
        return mNear;
    }

    public double getBlurAtInfinity() {
        return mBlurAtInfinity;
    }

    public double getFocalDistance() {
        return mFocalDistance;
    }

    public double getDepthOfField() {return mDepthOfFiled; }

    public double getFocalPointX() {
        return mFocalPointX;
    }

    public double getFocalPointY() {
        return mFocalPointY;
    }
}

