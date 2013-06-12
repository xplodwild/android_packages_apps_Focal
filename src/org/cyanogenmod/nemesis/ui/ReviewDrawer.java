package org.cyanogenmod.nemesis.ui;

import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.net.Uri;
import android.os.Handler;
import android.provider.MediaStore;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.DecelerateInterpolator;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;

import org.cyanogenmod.nemesis.R;
import org.cyanogenmod.nemesis.Util;

import java.util.ArrayList;
import java.util.List;

/**
 * This class handles the review drawer that can be opened by swiping down
 */
public class ReviewDrawer extends LinearLayout {
    public final static String TAG = "ReviewDrawer";

    private final static long DRAWER_TOGGLE_DURATION = 400;
    private final static int DRAWER_QUICK_REVIEW_SIZE_DP = 120;
    private final static String GALLERY_CAMERA_BUCKET = "Camera";
    private int mThumbnailSize;

    private List<Integer> mImages;

    private Handler mHandler;
    private ListView mImagesList;
    private ImageView mReviewedImage;
    private int mReviewedImageOrientation;
    private ImageListAdapter mImagesListAdapter;
    private Bitmap mReviewedBitmap;
    private int mReviewedImageId;
    private int mCurrentOrientation;
    private boolean mIsOpen;

    public ReviewDrawer(Context context) {
        super(context);
        initialize();
    }

    public ReviewDrawer(Context context, AttributeSet attrs) {
        super(context, attrs);
        initialize();
    }

    public ReviewDrawer(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        initialize();
    }

    private void initialize() {
        mHandler = new Handler();
        mImages = new ArrayList<Integer>();
        mThumbnailSize = getContext().getResources().getDimensionPixelSize(R.dimen.review_drawer_thumb_size);

        // Default hidden
        setAlpha(0.0f);
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                setTranslationX(-getMeasuredWidth());
            }
        });

        // Setup the list adapter
        mImagesListAdapter = new ImageListAdapter();

    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mImagesList = (ListView) findViewById(R.id.list_reviewed_images);
        mImagesList.setAdapter(mImagesListAdapter);
        mImagesList.setOnItemClickListener(new ImageListClickListener());

        mReviewedImage = (ImageView) findViewById(R.id.reviewed_image);
        mReviewedImage.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                openInGallery(mReviewedImageId);
            }
        });

        // Load pictures from gallery
        updateFromGallery();

        // Make sure drawer is initially closed
        setTranslationX(-9999);
    }

    public void updateFromGallery() {
        new Thread() {
            public void run() {
                mImages.clear();

                final String[] columns = { MediaStore.Images.Media.DATA, MediaStore.Images.Media._ID };
                final String orderBy = MediaStore.Images.Media.DATE_TAKEN + " ASC";

                // Select only the images that has been taken from the Camera
                final Cursor cursor = getContext().getContentResolver().query(
                        MediaStore.Images.Media.EXTERNAL_CONTENT_URI, columns, MediaStore.Images.Media.BUCKET_DISPLAY_NAME + " LIKE ?",
                        new String[] { GALLERY_CAMERA_BUCKET }, orderBy);

                if (cursor == null) {
                    Log.e(TAG, "Null cursor from MediaStore!");
                    return;
                }

                final int imageColumnIndex = cursor.getColumnIndex(MediaStore.Images.Media._ID);

                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        for (int i = 0; i < cursor.getCount(); i++) {
                            cursor.moveToPosition(i);

                            int id = cursor.getInt(imageColumnIndex);
                            addImageToList(id);
                        }
                    }
                });

                if (cursor.getCount() > 0) {
                    // Set the default reviewed image to the last image
                    cursor.moveToFirst();
                    final int firstPictureId = cursor.getInt(imageColumnIndex);
                    setPreviewedImage(firstPictureId);
                }
            }
        }.start();
    }

    /**
     * Queries the Media service for image orientation
     * @param id The id of the gallery image
     * @return The orientation of the image, or 0 if it failed
     */
    public int getCameraPhotoOrientation(final int id){
        Uri uri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI.buildUpon()
                .appendPath(Integer.toString(id)).build();

        String[] orientationColumn = {MediaStore.Images.Media.ORIENTATION};
        Cursor cur = getContext().getContentResolver().query(uri, orientationColumn, null, null, null);
        if (cur != null && cur.moveToFirst()) {
            return cur.getInt(cur.getColumnIndex(orientationColumn[0]));
        } else {
            return 0;
        }
    }

    /**
     * Sets the previewed image in the review drawer
     * @param id The id the of the image
     */
    public void setPreviewedImage(final int id) {
        new Thread() {
            public void run() {
                if (mReviewedBitmap != null)
                    mReviewedBitmap.recycle();
                mReviewedImageId = id;
                mReviewedBitmap = MediaStore.Images.Thumbnails.getThumbnail(
                        getContext().getContentResolver(), id,
                        MediaStore.Images.Thumbnails.MINI_KIND, null);

                mReviewedImageOrientation = getCameraPhotoOrientation(id);

                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        mReviewedImage.setImageBitmap(mReviewedBitmap);
                        notifyOrientationChanged(mCurrentOrientation);
                    }
                });
            }
        }.start();
    }

    public void addImageToList(int id) {
        mImagesListAdapter.addImage(id);
    }

    public void notifyOrientationChanged(int orientation) {
        mCurrentOrientation = orientation;
        mReviewedImage.animate().rotation(mReviewedImageOrientation + orientation)
                .setDuration(200).setInterpolator(new DecelerateInterpolator())
                .start();
    }

    private void openInGallery(int imageId) {
        if (imageId > 0) {
            Uri uri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI.buildUpon().appendPath(Integer.toString(imageId)).build();
            Intent intent = new Intent(Intent.ACTION_VIEW, uri);
            getContext().startActivity(intent);
        }
    }

    /**
     * Sets the review drawer to temporary hide, by reducing alpha to a very low
     * level
     * @param enabled To hide or not to hide, that is the question
     */
    public void setTemporaryHide(boolean enabled) {
        float alpha = (enabled ? 0.2f : 1.0f);
        animate().alpha(alpha).setDuration(200).start();
    }

    /**
     * Returns whether or not the review drawer is FULLY open (ie. not in
     * quick review mode)
     */
    public boolean isOpen() {
        return mIsOpen;
    }

    /**
     * Normally opens the review drawer (animation)
     */
    public void open() {
        mIsOpen = true;
        animate().setDuration(DRAWER_TOGGLE_DURATION).setInterpolator(new AccelerateInterpolator())
                .translationX(0.0f).alpha(1.0f).start();
    }

    /**
     * Normally closes the review drawer (animation)
     */
    public void close() {
        mIsOpen = false;
        animate().setDuration(DRAWER_TOGGLE_DURATION).setInterpolator(new DecelerateInterpolator())
                .translationX(-getMeasuredWidth()).alpha(0.0f).start();
    }

    public void slide(float distance) {
        setTranslationX(getTranslationX() + distance);

        if (getAlpha() == 0.0f) {
            setAlpha(1.0f);
        }
    }

    public void clampSliding() {
        if (getTranslationX() < -getMeasuredWidth()/2) {
            close();
        } else {
            open();
        }
    }

    /**
     * Open the drawer in Quick Review mode
     */
    public void openQuickReview() {
        float widthPx = Util.dpToPx(getContext(), DRAWER_QUICK_REVIEW_SIZE_DP);
        animate().setDuration(DRAWER_TOGGLE_DURATION).setInterpolator(new AccelerateInterpolator())
                .translationX(-getMeasuredWidth() + widthPx).alpha(1.0f).start();
    }


    /**
     * Class that handles the clicks on the image list
     */
    private class ImageListClickListener implements AdapterView.OnItemClickListener {
        @Override
        public void onItemClick(AdapterView<?> adapterView, View view, int i, long l) {
            int imageId = mImages.get(i);
            setPreviewedImage(imageId);
            mReviewedImageId = imageId;
        }
    }


    /**
     * Adapter responsible for showing the images in the list of the review drawer
     */
    private class ImageListAdapter extends BaseAdapter {
        public ImageListAdapter() {

        }

        public void addImage(int id) {
            mImages.add(0, id);
            mHandler.post(new Runnable() {
                public void run() {
                    ImageListAdapter.this.notifyDataSetChanged();
                }
            });
        }

        @Override
        public boolean areAllItemsEnabled() {
            return true;
        }

        @Override
        public boolean isEnabled(int i) {
            return true;
        }


        @Override
        public int getCount() {
            return mImages.size();
        }

        @Override
        public Integer getItem(int i) {
            return mImages.get(i);
        }

        @Override
        public long getItemId(int i) {
            return mImages.get(i);
        }

        @Override
        public View getView(final int i, View convertView, ViewGroup viewGroup) {
            ImageView imageView;

            // Recycle the view if any exists, remove and recycle its bitmap
            if (convertView == null) {
                imageView = new ImageView(getContext());
            } else {
                imageView = (ImageView) convertView;
                BitmapDrawable bitmapDrawable = ((BitmapDrawable) imageView.getDrawable());

                if (bitmapDrawable != null) {
                    Bitmap bmp = bitmapDrawable.getBitmap();
                    if (bmp != null)
                        bmp.recycle();
                }

                imageView.setImageBitmap(null);
            }

            // Hide the image for a nice transition while scrolling
            imageView.setAlpha(0.0f);
            imageView.setMinimumHeight(mThumbnailSize);

            // Thread the loading to not hang the UI
            final ImageView finalImageView = imageView;
            new Thread() {
                public void run() {
                    final Bitmap thumbnail = MediaStore.Images.Thumbnails.getThumbnail(
                            getContext().getContentResolver(), mImages.get(i),
                            MediaStore.Images.Thumbnails.MICRO_KIND, null);

                    if (thumbnail == null) {
                        Log.e(TAG, "Thumbnail is null!");
                        return;
                    }

                    // Thumbnail is ready, apply it to the ImageView in the UI thread and animate it
                    mHandler.post(new Runnable() {
                        public void run() {
                            // If we shoot really fast, the thumbnail might be recycled
                            // even before we displayed it, resulting in a crash
                            if (!thumbnail.isRecycled()) {
                                finalImageView.setImageBitmap(thumbnail);
                            }

                            finalImageView.animate().alpha(1.0f).setDuration(100).start();
                        }
                    });

                }
            }.start();

            return imageView;
        }


    }
}
