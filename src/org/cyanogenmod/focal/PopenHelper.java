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

package org.cyanogenmod.focal;

/**
 * Helper class calling a JNI native popen method to run system
 * commands
 */
public class PopenHelper {
    static {
        System.loadLibrary("xmptoolkit");
        System.loadLibrary("popen_helper_jni");
    }

    /**
     * Runs the specified command-line program in the command parameter.
     * Note that this command is ran with the shell interpreter (/system/bin/sh), thus
     * a shell script or a bunch of commands can be called in one method call.
     * The output of the commands (stdout/stderr) is sent out to the logcat with the
     * INFORMATION level.
     *
     * @params command The command to run
     * @return The call return code
     */
    public static native int run(String command);
}
