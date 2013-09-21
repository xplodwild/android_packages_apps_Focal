/*
 * Copyright (C) 2013 Guillaume Lesniak
 * Copyright (C) 2012 Paul Lawitzki
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

package org.cyanogenmod.focal.picsphere;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;

import java.util.Timer;
import java.util.TimerTask;

public class SensorFusion implements SensorEventListener {
    private SensorManager mSensorManager = null;
    float[] mOrientation = new float[3];
    float[] mRotationMatrix;

    public SensorFusion(Context context) {
        // get sensorManager and initialise sensor listeners
        mSensorManager = (SensorManager) context.getSystemService(Context.SENSOR_SERVICE);
        initListeners();
    }

    public void onPauseOrStop() {
        mSensorManager.unregisterListener(this);
    }

    public void onResume() {
        // restore the sensor listeners when user resumes the application.
        initListeners();
    }

    // This function registers sensor listeners for the accelerometer, magnetometer and gyroscope.
    public void initListeners(){
        mSensorManager.registerListener(this,
                mSensorManager.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR),
                20000);
    }

    @Override
    public void onSensorChanged(SensorEvent event) {
        if (mRotationMatrix == null) {
            mRotationMatrix = new float[16];
        }

        // Get rotation matrix from angles
        SensorManager.getRotationMatrixFromVector(mRotationMatrix, event.values);

        // Remap the axes
        SensorManager.remapCoordinateSystem(mRotationMatrix, SensorManager.AXIS_MINUS_Z,
                SensorManager.AXIS_X, mRotationMatrix);

        // Get back a remapped orientation vector
        SensorManager.getOrientation(mRotationMatrix, mOrientation);
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int i) {

    }

    public float[] getFusedOrientation() {
        return mOrientation;
    }

    public float[] getRotationMatrix() {
        return mRotationMatrix;
    }
}
