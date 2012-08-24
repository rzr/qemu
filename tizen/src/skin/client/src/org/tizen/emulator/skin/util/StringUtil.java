/**
 * 
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * HyunJun Son <hj79.son@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
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
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

package org.tizen.emulator.skin.util;

import java.io.File;
import java.io.IOException;

/**
 * 
 *
 */
public class StringUtil {
	
	private StringUtil(){}
	
	public static boolean isEmpty( String value ) {
		return ( null == value ) || ( 0 == value.length() );
	}

	public static String nvl( String value ) {
		return ( null == value ) ? "" : value;
	}

	public static String getCanonicalPath(String filePath) throws IOException {
		String canonicalPath = "";

		File file = new File(filePath);
		if (file.exists() == false) {
			return "";
		}

		try {
			canonicalPath = file.getCanonicalPath();

			if (file.isDirectory() == false) {
				canonicalPath =
						canonicalPath.substring(0, canonicalPath.lastIndexOf(File.separator));
			}
		} catch (IOException e) {
			throw e;
		}

		return canonicalPath;
	}
}
