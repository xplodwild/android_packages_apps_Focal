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

package org.cyanogenmod.focal.feats;

import org.cyanogenmod.focal.CameraManager;
import org.cyanogenmod.focal.SnapshotManager;
import org.cyanogenmod.focal.ui.ShutterButton;

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

    /**
     * Triggers a secondary action when the shutter button is long-pressed
     * (optional)
     */
    public void onShutterButtonLongPressed(ShutterButton button) { }

}
