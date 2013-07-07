/**
 * Copyright (C) 2013 The CyanogenMod Project
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

package org.cyanogenmod.nemesis.feats;

import org.cyanogenmod.nemesis.CameraManager;
import org.cyanogenmod.nemesis.SnapshotManager;
import org.cyanogenmod.nemesis.ui.ShutterButton;

/**
 * Handles the timer before a capture, or the voice shutter feature
 */
public class TimerCapture extends CaptureTransformer {
    public final static int VOICE_TIMER_VALUE= -1;

    public TimerCapture(CameraManager camMan, SnapshotManager snapshotMan) {
        super(camMan, snapshotMan);
    }

    /**
     * Sets the amount of seconds before taking a shot, or VOICE_TIMER_VALUE for voice
     * trigger commands
     *
     * @param seconds
     */
    public void setTimer(int seconds) {

    }

    @Override
    public void onShutterButtonClicked(ShutterButton button) {

    }

    @Override
    public void onSnapshotShutter(SnapshotManager.SnapshotInfo info) {

    }

    @Override
    public void onSnapshotPreview(SnapshotManager.SnapshotInfo info) {

    }

    @Override
    public void onSnapshotProcessed(SnapshotManager.SnapshotInfo info) {

    }

    @Override
    public void onSnapshotSaved(SnapshotManager.SnapshotInfo info) {

    }

    @Override
    public void onMediaSavingStart() {

    }

    @Override
    public void onMediaSavingDone() {

    }

    @Override
    public void onVideoRecordingStart() {

    }

    @Override
    public void onVideoRecordingStop() {

    }
}
