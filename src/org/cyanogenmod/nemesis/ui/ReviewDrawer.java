package org.cyanogenmod.nemesis.ui;

import android.content.Context;
import android.database.Cursor;
import android.database.DataSetObserver;
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
import android.widget.*;
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

    private Handler mHandler;
    private ListView mImagesList;
    private ImageView mReviewedImage;
    private ImageListAdapter mImagesListAdapter;

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

        // Load pictures from gallery
        updateFromGallery();
    }

    private void updateFromGallery() {
        final String[] columns = { MediaStore.Images.Media.DATA, MediaStore.Images.Media._ID };
        final String orderBy = MediaStore.Images.Media.DATE_TAKEN + " DESC";

        // Select only the images that has been taken from the Camera
        Cursor cursor = getContext().getContentResolver().query(
                MediaStore.Images.Media.EXTERNAL_CONTENT_URI, columns, MediaStore.Images.Media.BUCKET_DISPLAY_NAME + " LIKE ?",
                new String[] { "Camera" }, orderBy);

        if (cursor == null) {
            Log.e(TAG, "Null cursor from MediaStore!");
            return;
        }

        int imageColumnIndex = cursor.getColumnIndex(MediaStore.Images.Media._ID);

        for (int i = 0; i < cursor.getCount(); i++) {
            cursor.moveToPosition(i);

            int id = cursor.getInt(imageColumnIndex);
            addImageToList(id);
        }

        // Set the default reviewed image to the last image
        cursor.moveToFirst();
        final int firstPictureId = cursor.getInt(imageColumnIndex);

        new Thread() {
            public void run() {
                final Bitmap thumbnail = MediaStore.Images.Thumbnails.getThumbnail(
                        getContext().getContentResolver(), firstPictureId,
                        MediaStore.Images.Thumbnails.MINI_KIND, null);
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        mReviewedImage.setImageBitmap(thumbnail);
                    }
                });
            }
        }.start();
    }

    public void addImageToList(int id) {
        mImagesListAdapter.addImage(id);
    }

    /**
     * Normally opens the review drawer (animation)
     */
    public void open() {
        animate().setDuration(DRAWER_TOGGLE_DURATION).setInterpolator(new AccelerateInterpolator())
                .translationX(0.0f).alpha(1.0f).start();
    }

    /**
     * Normally closes the review drawer (animation)
     */
    public void close() {
        animate().setDuration(DRAWER_TOGGLE_DURATION).setInterpolator(new DecelerateInterpolator())
                .translationX(-getMeasuredWidth()).alpha(0.0f).start();
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

        }
    }


    /**
     * Adapter responsible for showing the images in the list of the review drawer
     */
    private class ImageListAdapter implements ListAdapter {
        private List<Integer> mImages;

        public ImageListAdapter() {
            mImages = new ArrayList<Integer>();
        }

        public void addImage(int id) {
            mImages.add(id);
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
        public void registerDataSetObserver(DataSetObserver dataSetObserver) {

        }

        @Override
        public void unregisterDataSetObserver(DataSetObserver dataSetObserver) {

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
            return i;
        }

        @Override
        public boolean hasStableIds() {
            return true;
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

                    // Thumbnail is ready, apply it to the ImageView in the UI thread and animate it
                    mHandler.post(new Runnable() {
                        public void run() {
                            finalImageView.setImageBitmap(thumbnail);
                            finalImageView.animate().alpha(1.0f).setDuration(100).start();
                        }
                    });

                }
            }.start();

            return imageView;
        }

        @Override
        public int getItemViewType(int i) {
            return 0;
        }

        @Override
        public int getViewTypeCount() {
            return 1;
        }

        @Override
        public boolean isEmpty() {
            return (mImages.size() == 0);
        }
    }
}
