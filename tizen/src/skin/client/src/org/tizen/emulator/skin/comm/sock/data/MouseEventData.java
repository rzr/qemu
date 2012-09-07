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

	int mouseButton;
	int eventType;
	int hostX;
	int hostY;
	int guestX;
	int guestY;
	int z;

	public MouseEventData(int mouseButton, int eventType,
			int hostX, int hostY, int guestX, int guestY, int z) {
		this.mouseButton = mouseButton;
		this.eventType = eventType;
		this.hostX = hostX;
		this.hostY = hostY;
		this.guestX = guestX;
		this.guestY = guestY;
		this.z = z;
	}

	@Override
	protected void write() throws IOException {
		writeInt(mouseButton);
		writeInt(eventType);
		writeInt(hostX);
		writeInt(hostY);
		writeInt(guestX);
		writeInt(guestY);
		writeInt(z);
	}

	@Override
	public String toString() {
		StringBuilder builder = new StringBuilder();
		builder.append("MouseEventData [mouseButton=");
		builder.append(mouseButton);
		builder.append(", eventType=");
		builder.append(eventType);
		builder.append(", hostX=");
		builder.append(hostX);
		builder.append(", hostY=");
		builder.append(hostY);
		builder.append(", guestX=");
		builder.append(guestX);
		builder.append(", guestY=");
		builder.append(guestY);
		builder.append(", id=");
		builder.append(z);
		builder.append("]");
		return builder.toString();
	}

}
