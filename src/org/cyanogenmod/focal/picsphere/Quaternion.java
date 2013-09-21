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

package org.cyanogenmod.focal.picsphere;

import android.opengl.Matrix;

/**
 * 3D Maths - Quaternion
 * Inspired from http://content.gpwiki.org/index.php/OpenGL%3aTutorials%3aUsing_Quaternions_to_represent_rotation
 */
public class Quaternion {
    public float x;
    public float y;
    public float z;
    public float w = 1.0f;

    public final static float PIOVER180 = (float) (Math.PI / 180.0f);
    private final static float TOLERANCE = 0.00001f;

    public Quaternion() {

    }

    public Quaternion(Quaternion o) {
        this.x = o.x;
        this.y = o.y;
        this.z = o.z;
        this.w = o.w;
    }

    public Quaternion(float x, float y, float z, float w) {
        this.x = x;
        this.y = y;
        this.z = z;
        this.w = w;
    }

    public void fromEuler(float pitch, float yaw, float roll) {
        // Basically we create 3 Quaternions, one for pitch, one for yaw, one for roll
        // and multiply those together.
        // the calculation below does the same, just shorter

        float p = pitch * PIOVER180 / 2.0f;
        float y = yaw * PIOVER180 / 2.0f;
        float r = roll * PIOVER180 / 2.0f;

        float sinp = (float) Math.sin(p);
        float siny = (float) Math.sin(y);
        float sinr = (float) Math.sin(r);
        float cosp = (float) Math.cos(p);
        float cosy = (float) Math.cos(y);
        float cosr = (float) Math.cos(r);

        this.x = sinr * cosp * cosy - cosr * sinp * siny;
        this.y = cosr * sinp * cosy + sinr * cosp * siny;
        this.z = cosr * cosp * siny - sinr * sinp * cosy;
        this.w = cosr * cosp * cosy + sinr * sinp * siny;

        normalise();
    }

    // Convert to Matrix
    public float[] getMatrix() {
        float x2 = x * x;
        float y2 = y * y;
        float z2 = z * z;
        float xy = x * y;
        float xz = x * z;
        float yz = y * z;
        float wx = w * x;
        float wy = w * y;
        float wz = w * z;

        // This calculation would be a lot more complicated for non-unit length quaternions
        // Note: The constructor of Matrix4 expects the Matrix in column-major format like
        // expected by OpenGL
        return new float[]{1.0f - 2.0f * (y2 + z2), 2.0f * (xy - wz), 2.0f * (xz + wy), 0.0f,
                2.0f * (xy + wz), 1.0f - 2.0f * (x2 + z2), 2.0f * (yz - wx), 0.0f,
                2.0f * (xz - wy), 2.0f * (yz + wx), 1.0f - 2.0f * (x2 + y2), 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f};
    }

    public Quaternion getConjugate() {
        return new Quaternion(-x, -y, -z, w);
    }

    // Multiplying q1 with q2 applies the rotation q2 to q1
    public Quaternion multiply(Quaternion rq) {
        // the constructor takes its arguments as (x, y, z, w)
        return new Quaternion(w * rq.x + x * rq.w + y * rq.z - z * rq.y,
                w * rq.y + y * rq.w + z * rq.x - x * rq.z,
                w * rq.z + z * rq.w + x * rq.y - y * rq.x,
                w * rq.w - x * rq.x - y * rq.y - z * rq.z);
    }

    // normalising a quaternion works similar to a vector. This method will not do anything
    // if the quaternion is close enough to being unit-length. define TOLERANCE as something
    // small like 0.00001f to get accurate results
    public void normalise() {
        // Don't normalize if we don't have to
        float mag2 = w * w + x * x + y * y + z * z;
        if (Math.abs(mag2) > TOLERANCE && Math.abs(mag2 - 1.0f) > TOLERANCE) {
            float mag = (float) Math.sqrt(mag2);
            w /= mag;
            x /= mag;
            y /= mag;
            z /= mag;
        }
    }
}
