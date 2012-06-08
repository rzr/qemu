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

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;

/**
 * 
 *
 */
public abstract class AbstractSendData implements ISendData {
	
	private DataOutputStream dos;
	private ByteArrayOutputStream bao;
	
	public AbstractSendData() {
		bao = new ByteArrayOutputStream();
		dos = new DataOutputStream( bao );
	}
	
	protected abstract void write() throws IOException ;
	
	@Override
	public byte[] serialize() throws IOException {
		write();
		return bao.toByteArray();
	}

	protected void writeShort ( int val ) throws IOException {
		dos.writeShort( val );
	}

	protected void writeInt ( int val ) throws IOException {
		dos.writeInt( val );
	}

	protected void writeLong ( long val ) throws IOException {
		dos.writeLong( val );
	}

	protected void writeBoolean ( boolean val ) throws IOException {
		dos.writeBoolean( val );
	}

	protected void writeChar ( char val ) throws IOException {
		dos.writeChar( val );
	}

}
