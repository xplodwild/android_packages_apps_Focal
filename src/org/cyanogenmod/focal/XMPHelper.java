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
 * XMP Helper to write XMP data to files
 */
public class XMPHelper {
    static {
        System.loadLibrary("xmptoolkit");
        System.loadLibrary("xmphelper_jni");
    }

    /**
     * Writes the provided XMP Data to the specified fileName. This goes through
     * Adobe's XMP toolkit library.
     *
     * @param fileName The file name to which write the XMP metadata
     * @param xmpData The RDF-formatted data
     * @return -1 if error, 0 if everything went fine
     */
    public native int writeXmpToFile(String fileName, String xmpData);
}
