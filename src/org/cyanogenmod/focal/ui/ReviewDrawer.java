/*
 * Copyright (C) 2013 Guillaume Lesniak
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

package org.cyanogenmod.focal.ui;

import android.animation.Animator;
import android.content.ActivityNotFoundException;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Handler;
import android.provider.MediaStore;
import android.support.v4.view.ViewPager;
import android.util.AttributeSet;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.DecelerateInterpolator;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;

import org.cyanogenmod.focal.CameraActivity;
import fr.xplod.focal.R;
import org.cyanogenmod.focal.Util;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * This class handles the review drawer that can be opened by swiping down
 */
public class ReviewDrawer extends RelativeLayout {
    public final static String TAG = "ReviewDrawer";

    private final static long DRAWER_TOGGLE_DURATION = 400;
    private final static float MIN_REMOVE_THRESHOLD = 10.0f;
    private final static String GALLERY_CAMERA_BUCKET = "Camera";

    private List<Integer> mImages;

    private Handler mHandler;
    private ImageListAdapter mImagesListAdapter;
    private int mReviewedImageId;
    private int mCurrentOrientation;
    private final Object mImagesLock = new Object();
    private boolean mIsOpen;
    private ViewPager mViewPager;

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

        // Default hidden
        setAlpha(0.0f);
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                setTranslationY(-getMeasuredHeight());
            }
        });

        // Setup the list adapter
        mImagesListAdapter = new ImageListAdapter();
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        // Load pictures or videos from gallery
        if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_VIDEO) {
            updateFromGallery(false, 0);
        } else {
            updateFromGallery(true, 0);
        }

        // Make sure drawer is initially closed
        setTranslationY(-99999);

        ImageButton editImageButton = (ImageButton) findViewById(R.id.button_retouch);
        editImageButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                editInGallery(mReviewedImageId);
            }
        });

        ImageButton openImageButton = (ImageButton) findViewById(R.id.button_open_in_gallery);
        openImageButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                openInGallery(mReviewedImageId);
            }
        });

        mViewPager = (ViewPager) findViewById(R.id.reviewed_image);
        mViewPager.setAdapter(mImagesListAdapter);
        mViewPager.setPageMargin((int) Util.dpToPx(getContext(), 8));
        mViewPager.setPageTransformer(true, new ZoomOutPageTransformer());
        mViewPager.setOnPageChangeListener(new ViewPager.OnPageChangeListener() {
            @Override
            public void onPageScrolled(int i, float v, int i2) {

            }

            @Override
            public void onPageSelected(int i) {
                mReviewedImageId = mImages.get(i);
            }

            @Override
            public void onPageScrollStateChanged(int i) {

            }
        });
    }

    /**
     * Clears the list of images and reload it from the Gallery (MediaStore)
     * This method is threaded!
     *
     * @param images True to get images, false to get videos
     * @param scrollPos Position to scroll to, or 0 to get latest image
     */
    public void updateFromGallery(final boolean images, final int scrollPos) {
        new Thread() {
            public void run() {
                updateFromGallerySynchronous(images, scrollPos);
            }
        }.start();
    }

    /**
     * Clears the list of images and reload it from the Gallery (MediaStore)
     * This method is synchronous, see updateFromGallery for the threaded one.
     *
     * @param images True to get images, false to get videos
     * @param scrollPos Position to scroll to, or 0 to get latest image
     */
    public void updateFromGallerySynchronous(final boolean images, final int scrollPos) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                synchronized (mImagesLock) {
                    mImages.clear();
                    mImagesListAdapter.notifyDataSetChanged();
                }

                String[] columns;
                String orderBy;
                if (images) {
                    columns = new String[]{MediaStore.Images.Media.DATA, MediaStore.Images.Media._ID};
                    orderBy = MediaStore.Images.Media.DATE_TAKEN + " ASC";
                } else {
                    columns = new String[]{MediaStore.Video.Media.DATA, MediaStore.Video.Media._ID};
                    orderBy = MediaStore.Video.Media.DATE_TAKEN + " ASC";
                }

                // Select only the images that has been taken from the Camera
                Context ctx = getContext();
                if (ctx == null) return;
                ContentResolver cr = ctx.getContentResolver();
                if (cr == null) {
                    Log.e(TAG, "No content resolver!");
                    return;
                }

                Cursor cursor;

                if (images) {
                    cursor = cr.query(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, columns,
                            MediaStore.Images.Media.BUCKET_DISPLAY_NAME + " LIKE ?",
                            new String[]{GALLERY_CAMERA_BUCKET}, orderBy);
                } else {
                    cursor = cr.query(MediaStore.Video.Media.EXTERNAL_CONTENT_URI, columns,
                            MediaStore.Video.Media.BUCKET_DISPLAY_NAME + " LIKE ?",
                            new String[]{GALLERY_CAMERA_BUCKET}, orderBy);
                }

                if (cursor == null) {
                    Log.e(TAG, "Null cursor from MediaStore!");
                    return;
                }

                final int imageColumnIndex = cursor.getColumnIndex(images ?
                        MediaStore.Images.Media._ID : MediaStore.Video.Media._ID);

                for (int i = 0; i < cursor.getCount(); i++) {
                    cursor.moveToPosition(i);

                    int id = cursor.getInt(imageColumnIndex);
                    if (mReviewedImageId <= 0) {
                        mReviewedImageId = id;
                    }
                    addImageToList(id);
                    mImagesListAdapter.notifyDataSetChanged();
                }

                cursor.close();

                if (scrollPos < mImages.size()) {
                    mViewPager.setCurrentItem(scrollPos+1, false);
                    mViewPager.setCurrentItem(scrollPos, true);
                }
            }
        });
    }

    /**
     * Queries the Media service for image orientation
     *
     * @param id The id of the gallery image
     * @return The orientation of the image, or 0 if it failed
     */
    public int getCameraPhotoOrientation(final int id) {
        if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_VIDEO) {
            return 0;
        }
        Uri uri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI.buildUpon()
                .appendPath(Integer.toString(id)).build();
        String[] orientationColumn = new String[] {
                MediaStore.Images.ImageColumns.ORIENTATION
        };

        int orientation = 0;
        Cursor cur = getContext().getContentResolver().query(uri,
                orientationColumn, null, null, null);
        if (cur != null && cur.moveToFirst()) {
            orientation = cur.getInt(cur.getColumnIndex(orientationColumn[0]));
        }
        if (cur != null) {
            cur.close();
            cur = null;
        }
        return orientation;
    }


    /**
     * Add an image at the head of the image ribbon
     *
     * @param id The id of the image from the MediaStore
     */
    public void addImageToList(final int id) {
        mImagesListAdapter.addImage(id);
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                mImagesListAdapter.notifyDataSetChanged();
            }
        });
    }

    public void scrollToLatestImage() {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                mViewPager.setCurrentItem(0);
            }
        });
    }

    public void notifyOrientationChanged(final int orientation) {
        mCurrentOrientation = orientation;
    }

    private void openInGallery(final int imageId) {
        if (imageId > 0) {
            Uri uri;
            if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_VIDEO) {
                uri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI.buildUpon()
                        .appendPath(Integer.toString(imageId)).build();
            } else {
                uri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI.buildUpon()
                        .appendPath(Integer.toString(imageId)).build();
            }
            Intent intent = new Intent(Intent.ACTION_VIEW, uri);
            try {
                Context ctx = getContext();
                if (ctx != null) {
                    ctx.startActivity(intent);
                }
            } catch (ActivityNotFoundException e) {
                CameraActivity.notify(getContext().getString(R.string.no_video_player), 2000);
            }
        }
    }

    private void editInGallery(final int imageId) {
        if (imageId > 0) {
            Intent editIntent = new Intent(Intent.ACTION_EDIT);

            // Get URI
            Uri uri = null;
            if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_VIDEO) {
                uri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI.buildUpon()
                        .appendPath(Integer.toString(imageId)).build();
            } else {
                uri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI.buildUpon()
                        .appendPath(Integer.toString(imageId)).build();
            }

            // Start gallery edit activity
            editIntent.setDataAndType(uri, "image/*");
            editIntent.setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            Context ctx = getContext();
            if (ctx != null) {
                ctx.startActivity(Intent.createChooser(editIntent, null));
            }
        }
    }

    /**
     * Sets the review drawer to temporary hide, by reducing alpha to a very low
     * level
     *
     * @param enabled To hide or not to hide, that is the question
     */
    public void setTemporaryHide(final boolean enabled) {
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
        //mReviewedImage.setVisibility(View.VISIBLE);
        openImpl(1.0f);
    }

    private void openImpl(final float alpha) {
        mIsOpen = true;
        setVisibility(View.VISIBLE);
        animate().setDuration(DRAWER_TOGGLE_DURATION).setInterpolator(new AccelerateInterpolator())
                .translationY(0.0f).setListener(null).alpha(alpha).start();
    }

    /**
     * Normally closes the review drawer (animation)
     */
    public void close() {
        mIsOpen = false;
        animate().setDuration(DRAWER_TOGGLE_DURATION).setInterpolator(new DecelerateInterpolator())
                .translationY(-getMeasuredHeight()).alpha(0.0f)
                .setListener(new Animator.AnimatorListener() {
                    @Override
                    public void onAnimationStart(Animator animator) {

                    }

                    @Override
                    public void onAnimationEnd(Animator animator) {
                        setVisibility(View.GONE);
                    }

                    @Override
                    public void onAnimationCancel(Animator animator) {

                    }

                    @Override
                    public void onAnimationRepeat(Animator animator) {

                    }
                }).start();
    }

    /**
     * Slide the review drawer of the specified distance on the X axis
     *
     * @param distance The distance to slide
     */
    public void slide(final float distance) {
        float finalPos = getTranslationY() + distance;
        if (finalPos > 0) {
            finalPos = 0;
        }

        setTranslationY(finalPos);

        if (getAlpha() == 0.0f) {
            setVisibility(View.VISIBLE);
            setAlpha(1.0f);
        }
    }

    public void clampSliding() {
        if (getTranslationY() < -getMeasuredHeight() / 2) {
            close();
        } else {
            openImpl(getAlpha());
        }
    }

    /**
     * Removes the currently reviewed image from the
     * internal memory.
     */
    public void removeReviewedImage() {
        Util.removeFromGallery(getContext().getContentResolver(), mReviewedImageId);
        int position = mViewPager.getCurrentItem();
        if (CameraActivity.getCameraMode() == CameraActivity.CAMERA_MODE_VIDEO) {
            updateFromGallery(false, position);
        } else {
            updateFromGallery(true, position);
        }

        mHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                if (mImages.size() > 0) {
                    int imageId = mImages.get(0);
                    mReviewedImageId = imageId;
                }
            }
        }, 300);
        // XXX: Undo popup
    }

    /**
     * Open the drawer in Quick Review mode
     */
    public void openQuickReview() {
        openImpl(0.5f);
    }

    /**
     * Adapter responsible for showing the images in the list of the review drawer
     */
    private class ImageListAdapter extends android.support.v4.view.PagerAdapter {
        private Map<ImageView, Integer> mViewsToId;

        public ImageListAdapter() {
            mViewsToId = new HashMap<ImageView, Integer>();
        }

        public void addImage(int id) {
            synchronized (mImagesLock) {
                mImages.add(0, id);
            }
        }

        @Override
        public int getItemPosition(Object object) {
            ImageView img = (ImageView) object;

            if (mImages.indexOf(mViewsToId.get(img)) >= 0) {
                return mImages.indexOf(mViewsToId.get(img));
            } else {
                return POSITION_NONE;
            }
        }

        @Override
        public int getCount() {
            return mImages.size();
        }

        @Override
        public boolean isViewFromObject(View view, Object o) {
            return view == o;
        }

        @Override
        public Object instantiateItem(ViewGroup container, final int position) {
            final ImageView imageView = new ImageView(getContext());
            container.addView(imageView);

            ViewGroup.LayoutParams params = imageView.getLayoutParams();
            params.width = ViewGroup.LayoutParams.WRAP_CONTENT;
            params.height = ViewGroup.LayoutParams.WRAP_CONTENT;
            imageView.setLayoutParams(params);

            imageView.setOnTouchListener(new ThumbnailTouchListener(imageView));

            new Thread() {
                public void run() {
                    int imageId = -1;
                    synchronized (mImagesLock) {
                        try {
                            imageId = mImages.get(position);
                        } catch (IndexOutOfBoundsException e) {
                            return;
                        }
                    }

                    mViewsToId.put(imageView, imageId);
                    final Bitmap thumbnail = CameraActivity.getCameraMode() ==
                            CameraActivity.CAMERA_MODE_VIDEO ? (MediaStore.Video.Thumbnails
                            .getThumbnail(getContext().getContentResolver(),
                                    mImages.get(position), MediaStore.Video.Thumbnails.MINI_KIND, null))
                            : (MediaStore.Images.Thumbnails.getThumbnail(getContext()
                            .getContentResolver(), imageId,
                            MediaStore.Images.Thumbnails.MINI_KIND, null));

                    final int rotation = getCameraPhotoOrientation(imageId);
                    mHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            imageView.setImageBitmap(thumbnail);
                            imageView.setRotation(rotation);
                        }
                    });
                }
            }.start();

            return imageView;
        }

        @Override
        public void destroyItem(ViewGroup container, int position, Object object) {
            // Remove viewpager_item.xml from ViewPager
            container.removeView((ImageView) object);

        }
    }

    /**
     * Pager transition animation
     */
    public class ZoomOutPageTransformer implements ViewPager.PageTransformer {
        private final float MIN_SCALE = 0.85f;
        private final float MIN_ALPHA = 0.5f;

        public void transformPage(View view, float position) {
            int pageWidth = view.getWidth();
            int pageHeight = view.getHeight();

            if (position < -1) { // [-Infinity,-1)
                // This page is way off-screen to the left.
                view.setAlpha(0);

            } else if (position <= 1) { // [-1,1]
                // Modify the default slide transition to shrink the page as well
                float scaleFactor = Math.max(MIN_SCALE, 1 - Math.abs(position));
                float vertMargin = pageHeight * (1 - scaleFactor) / 2;
                float horzMargin = pageWidth * (1 - scaleFactor) / 2;
                if (position < 0) {
                    view.setTranslationX(horzMargin - vertMargin / 2);
                } else {
                    view.setTranslationX(-horzMargin + vertMargin / 2);
                }

                // Scale the page down (between MIN_SCALE and 1)
                view.setScaleX(scaleFactor);
                view.setScaleY(scaleFactor);

                // Fade the page relative to its size.
                view.setAlpha(MIN_ALPHA +
                        (scaleFactor - MIN_SCALE) /
                                (1 - MIN_SCALE) * (1 - MIN_ALPHA));

            } else { // (1,+Infinity]
                // This page is way off-screen to the right.
                view.setAlpha(0);
            }
        }
    }

    public class ThumbnailTouchListener implements OnTouchListener {
        private final GestureDetector mGestureDetector;
        private ImageView mImageView;
        private GestureDetector.SimpleOnGestureListener mListener =
                new GestureDetector.SimpleOnGestureListener() {
                    private final float DRIFT_THRESHOLD = 80.0f;
                    private final int SWIPE_THRESHOLD_VELOCITY = 800;

                    @Override
                    public boolean onDown(MotionEvent motionEvent) {
                        return true;
                    }

                    @Override
                    public void onShowPress(MotionEvent motionEvent) {

                    }

                    @Override
                    public boolean onSingleTapUp(MotionEvent motionEvent) {
                        return false;
                    }

                    @Override
                    public boolean onScroll(MotionEvent ev1, MotionEvent ev2, float vX, float vY) {
                        if (Math.abs(ev2.getX() - ev1.getX()) > DRIFT_THRESHOLD
                                && Math.abs(mImageView.getTranslationY()) < MIN_REMOVE_THRESHOLD) {
                            return false;
                        }

                        mImageView.setTranslationY(ev2.getRawY() - ev1.getRawY());
                        float alpha = Math.max(0.0f, 1.0f - Math.abs(mImageView.getTranslationY()
                                / mImageView.getMeasuredHeight())*2.0f);
                        mImageView.setAlpha(alpha);

                        return true;
                    }

                    @Override
                    public void onLongPress(MotionEvent motionEvent) {

                    }

                    @Override
                    public boolean onFling(MotionEvent ev1, MotionEvent ev2, float vX, float vY) {
                        if (Math.abs(ev2.getX() - ev1.getX()) > DRIFT_THRESHOLD) {
                            return false;
                        }

                        if (Math.abs(vY) > SWIPE_THRESHOLD_VELOCITY) {
                            mImageView.animate().translationY(-mImageView.getHeight()).alpha(0.0f)
                                    .setDuration(300).start();
                            removeReviewedImage();
                            return true;
                        }

                        return false;
                    }
                };

        public ThumbnailTouchListener(ImageView iv) {
            mImageView = iv;
            mGestureDetector = new GestureDetector(getContext(), mListener);
        }

        @Override
        public boolean onTouch(View view, MotionEvent motionEvent) {
            if (motionEvent.getActionMasked() == MotionEvent.ACTION_UP) {
                if (mImageView.getTranslationY() > mImageView.getMeasuredHeight()*0.5f) {
                    mImageView.animate().translationY(-mImageView.getHeight()).alpha(0.0f)
                            .setDuration(300).start();
                    mHandler.postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            mImageView.setTranslationY(0.0f);
                            mImageView.setTranslationX(mImageView.getWidth());
                            mImageView.animate().translationX(0.0f).alpha(1.0f).start();
                        }
                    }, 400);
                    removeReviewedImage();
                } else {
                    mImageView.animate().translationY(0).alpha(1.0f)
                            .setDuration(300).start();
                }
            }

            return mGestureDetector.onTouchEvent(motionEvent);
        }
    }
}
