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
public class MouseEventData extends AbstractSendData {

	int eventType;
	int originX;
	int originY;
	int x;
	int y;
	int z;

	public MouseEventData(int eventType, int originX, int originY, int x, int y, int z) {
		this.eventType = eventType;
		this.originX = originX;
		this.originY = originY;
		this.x = x;
		this.y = y;
		this.z = z;
	}

	@Override
	protected void write() throws IOException {
		writeInt(eventType);
		writeInt(originX);
		writeInt(originY);
		writeInt(x);
		writeInt(y);
		writeInt(z);
	}

	@Override
	public String toString() {
		StringBuilder builder = new StringBuilder();
		builder.append("MouseEventData [eventType=");
		builder.append(eventType);
		builder.append(", originX=");
		builder.append(originX);
		builder.append(", originY=");
		builder.append(originY);
		builder.append(", transposeX=");
		builder.append(x);
		builder.append(", transposeY=");
		builder.append(y);
		builder.append(", id=");
		builder.append(z);
		builder.append("]");
		return builder.toString();
	}

}
