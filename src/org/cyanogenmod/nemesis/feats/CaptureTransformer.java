package org.cyanogenmod.nemesis.feats;

import org.cyanogenmod.nemesis.CameraManager;
import org.cyanogenmod.nemesis.SnapshotManager;
import org.cyanogenmod.nemesis.ui.ShutterButton;

/**
 * This class is a base class for all the effects/features affecting
 * the camera at capture, such as burst mode, or software HDR. It lets
 * you easily take more shots, process them, or throw them away, all of this
 * in one unique tap on the shutter button by the user.
 */
public abstract class CaptureTransformer implements SnapshotManager.SnapshotListener {
    protected CameraManager mCamManager;
    protected SnapshotManager mSnapManager;

    public CaptureTransformer(CameraManager camMan, SnapshotManager snapshotMan) {
        mCamManager = camMan;
        mSnapManager = snapshotMan;
    }

    /**
     * Triggers the logic of the CaptureTransformer, when the user
     * pressed the shutter button.
     */
    public abstract void onShutterButtonClicked(ShutterButton button);

}
