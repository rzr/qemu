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

package org.tizen.emulator.skin.comm.sock.data;

import java.io.IOException;

/**
 * 
 *
 */
public class StartData extends AbstractSendData {

	private long windowHandleId;
	private int lcdSizeWidth;
	private int lcdSizeHeight;
	private int scale;
	private short rotation;
	
	public StartData(long windowHandleId, int lcdSizeWidth, int lcdSizeHeight, int scale, short rotation ) {
		this.windowHandleId = windowHandleId;
		this.lcdSizeWidth = lcdSizeWidth;
		this.lcdSizeHeight = lcdSizeHeight;
		this.scale = scale;
		this.rotation = rotation;
	}

	@Override
	protected void write() throws IOException {
		writeLong( windowHandleId );
		writeInt( lcdSizeWidth );
		writeInt( lcdSizeHeight );
		writeInt( scale );
		writeShort( rotation );
	}

	@Override
	public String toString() {
		StringBuilder builder = new StringBuilder();
		builder.append("StartData [windowHandleId=");
		builder.append(windowHandleId);
		builder.append(", lcd size " + lcdSizeWidth +"x" + lcdSizeHeight);
		builder.append(", scale=");
		builder.append(scale);
		builder.append(", rotation=");
		builder.append(rotation);
		builder.append("]");
		return builder.toString();
	}

}
