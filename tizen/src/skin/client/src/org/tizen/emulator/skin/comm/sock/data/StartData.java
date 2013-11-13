/**
 * Initial Data From Skin
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 * HyunJun Son
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

package org.tizen.emulator.skin.comm.sock.data;

import java.io.IOException;

/**
 * 
 *
 */
public class StartData extends AbstractSendData {
	private long windowHandleId;
	private int displayWidth;
	private int displayHeight;
	private int scale;
	private short rotation;
	private boolean isBlankGuide;
	
	public StartData(long windowHandleId,
			int displayWidth, int displayHeight, int scale, short rotation,
			boolean isBlankGuide) {
		this.windowHandleId = windowHandleId;
		this.displayWidth = displayWidth;
		this.displayHeight = displayHeight;
		this.scale = scale;
		this.rotation = rotation;
		this.isBlankGuide = isBlankGuide;
	}

	@Override
	protected void write() throws IOException {
		writeLong(windowHandleId);
		writeInt(displayWidth);
		writeInt(displayHeight);
		writeInt(scale);
		writeShort(rotation);

		if (isBlankGuide == true) {
			writeShort(1);
		} else {
			writeShort(0);
		}
	}

	@Override
	public String toString() {
		StringBuilder builder = new StringBuilder();
		builder.append("StartData [windowHandleId=");
		builder.append(windowHandleId);
		builder.append(", display size " + displayWidth + "x" + displayHeight);
		builder.append(", scale=");
		builder.append(scale);
		builder.append(", rotation=");
		builder.append(rotation);
		builder.append(", blank guide=");
		builder.append(isBlankGuide);
		builder.append("]");

		return builder.toString();
	}
}
