/**
 * Communication Interface Between Qemu And Skin
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

package org.tizen.emulator.skin.comm;

import org.tizen.emulator.skin.comm.sock.data.ISendData;
import org.tizen.emulator.skin.dbi.RotationNameType;

/**
 * 
 *
 */
public interface ICommunicator extends Runnable {
	public enum MouseButtonType {
		LEFT( (short)1 ),
		WHEEL( (short)2 ),
		RIGHT( (short)3 );

		private short value;
		MouseButtonType( short value ) {
			this.value = value;
		}

		public short value() {
			return this.value;
		}

		public static MouseButtonType getValue( String val ) {
			MouseButtonType[] values = MouseButtonType.values();
			for (int i = 0; i < values.length; i++) {
				if( values[i].value == Integer.parseInt( val ) ) {
					return values[i];
				}
			}
			throw new IllegalArgumentException( val );
		}

		public static MouseButtonType getValue( short val ) {
			MouseButtonType[] values = MouseButtonType.values();
			for (int i = 0; i < values.length; i++) {
				if( values[i].value == val ) {
					return values[i];
				}
			}
			throw new IllegalArgumentException( Integer.toString(val) );
		}

	}

	public enum MouseEventType {
		PRESS( (short)1 ),
		RELEASE( (short)2 ),
		DRAG( (short)3 ),
		WHEELUP( (short)4 ),
		WHEELDOWN( (short)5 ),
		MOVE( (short)6 );

		private short value;
		MouseEventType( short value ) {
			this.value = value;
		}

		public short value() {
			return this.value;
		}

		public static MouseEventType getValue( String val ) {
			MouseEventType[] values = MouseEventType.values();
			for (int i = 0; i < values.length; i++) {
				if( values[i].value == Integer.parseInt( val ) ) {
					return values[i];
				}
			}
			throw new IllegalArgumentException( val );
		}

		public static MouseEventType getValue( short val ) {
			MouseEventType[] values = MouseEventType.values();
			for (int i = 0; i < values.length; i++) {
				if( values[i].value == val ) {
					return values[i];
				}
			}
			throw new IllegalArgumentException( Integer.toString(val) );
		}

	}

	public enum KeyEventType {
		PRESSED( (short)1 ),
		RELEASED( (short)2 );

		private short value;
		KeyEventType( short value ) {
			this.value = value;
		}

		public short value() {
			return this.value;
		}

		public static KeyEventType getValue( String val ) {
			KeyEventType[] values = KeyEventType.values();
			for (int i = 0; i < values.length; i++) {
				if( values[i].value == Integer.parseInt( val ) ) {
					return values[i];
				}
			}
			throw new IllegalArgumentException( val );
		}

		public static KeyEventType getValue( short val ) {
			KeyEventType[] values = KeyEventType.values();
			for (int i = 0; i < values.length; i++) {
				if( values[i].value == val ) {
					return values[i];
				}
			}
			throw new IllegalArgumentException( Integer.toString(val) );
		}

	}

	public enum RotationInfo {
		PORTRAIT( RotationNameType.PORTRAIT.value(), (short)0, 0 ),
		LANDSCAPE( RotationNameType.LANDSCAPE.value(), (short)1, -90 ),
		REVERSE_PORTRAIT( RotationNameType.REVERSE_PORTRAIT.value(), (short)2, 180 ),
		REVERSE_LANDSCAPE( RotationNameType.REVERSE_LANDSCAPE.value(), (short)3, 90 );

		private String value;
		private short id;
		private int angle;

		RotationInfo( String value, short id, int ratio ) {
			this.value = value;
			this.id = id;
			this.angle = ratio;
		}

		public String value() {
			return this.value;
		}

		public int angle() {
			return this.angle;
		}

		public short id() {
			return this.id;
		}

		public static RotationInfo getValue(short id) {
			RotationInfo[] values = RotationInfo.values();
			for (int i = 0; i < values.length; i++) {
				if (values[i].id == id) {
					return values[i];
				}
			}

			throw new IllegalArgumentException(Integer.toString(id));
		}
	}

	public enum SendCommand {
		/* This values must match the QEMU definitions */

		SEND_SKIN_OPENED((short) 1),

		SEND_MOUSE_EVENT((short) 10),
		SEND_KEYBOARD_KEY_EVENT((short) 11),
		SEND_HW_KEY_EVENT((short) 12),
		SEND_DISPLAY_STATE((short) 13),
		SEND_OPEN_SHELL((short) 14),
		SEND_HOST_KBD_STATE((short) 15),
		SEND_SCREENSHOT_REQ((short) 16),
		SEND_DETAIL_INFO_REQ((short) 17),
		SEND_RAM_DUMP((short) 18),
		SEND_GUEST_DUMP((short) 19),
		SEND_ECP_PORT_REQ((short) 20),

		RESPONSE_HEART_BEAT((short) 900),
		RESPONSE_DRAW_FRAME((short) 901),
		SEND_CLOSE_REQ((short) 998),
		RESPONSE_SHUTDOWN((short) 999);

		private short value;

		SendCommand(short value) {
			this.value = value;
		}

		public short value() {
			return this.value;
		}

		public static SendCommand getValue(String val) {
			SendCommand[] values = SendCommand.values();
			for (int i = 0; i < values.length; i++) {
				if (values[i].value == Short.parseShort(val)) {
					return values[i];
				}
			}
			throw new IllegalArgumentException(val);
		}

		public static SendCommand getValue(short val) {
			SendCommand[] values = SendCommand.values();
			for (int i = 0; i < values.length; i++) {
				if (values[i].value == val) {
					return values[i];
				}
			}
			throw new IllegalArgumentException(Integer.toString(val));
		}
	}

	public enum ReceiveCommand {
		/* This values must match the QEMU definitions */

		RECV_HEART_BEAT((short) 1),
		RECV_SCREENSHOT_DATA((short) 2),
		RECV_DETAIL_INFO_DATA((short) 3),
		RECV_RAMDUMP_COMPLETED((short) 4),
		RECV_BOOTING_PROGRESS((short) 5),
		RECV_BRIGHTNESS_STATE((short) 6),
		RECV_ECP_PORT_DATA((short) 7),
		RECV_HOST_KBD_STATE((short) 8),

		RECV_SENSORD_STARTED((short) 800),
		RECV_SDBD_STARTED((short) 801),
		RECV_ECS_STARTED((short) 802),
		RECV_DRAW_FRAME((short) 900),
		RECV_DRAW_BLANK_GUIDE((short) 901),
		RECV_SHUTDOWN((short) 999);

		private short value;

		ReceiveCommand(short value) {
			this.value = value;
		}

		public short value() {
			return this.value;
		}

		public static ReceiveCommand getValue(String val) {
			ReceiveCommand[] values = ReceiveCommand.values();
			for (int i = 0; i < values.length; i++) {
				if (values[i].value == Short.parseShort(val)) {
					return values[i];
				}
			}
			throw new IllegalArgumentException(val);
		}

		public static ReceiveCommand getValue(short val) {
			ReceiveCommand[] values = ReceiveCommand.values();
			for (int i = 0; i < values.length; i++) {
				if (values[i].value == val) {
					return values[i];
				}
			}
			throw new IllegalArgumentException(Integer.toString(val));
		}
	}

	public void sendToQEMU(SendCommand command, ISendData data, boolean urgent);

	public void terminate();
}
