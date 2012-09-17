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

package org.tizen.emulator.skin.comm;

import org.tizen.emulator.skin.comm.sock.data.ISendData;
import org.tizen.emulator.skin.dbi.RotationNameType;

/**
 * 
 *
 */
public interface ICommunicator extends Runnable {

	public enum Scale {
		SCALE_25(25),
		SCALE_50(50),
		SCALE_75(75),
		SCALE_100(100);

		private int value;

		Scale( int value ) {
			this.value = value;
		}

		public int value() {
			return this.value;
		}
	}
	
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
		DOWN( (short)1 ),
		UP( (short)2 ),
		DRAG( (short)3 ),
       WHEELUP( (short)4 ),
       WHEELDOWN( (short)5 );

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
		
		public static RotationInfo getValue( short id ) {
			RotationInfo[] values = RotationInfo.values();
			for (int i = 0; i < values.length; i++) {
				if( values[i].id == id ) {
					return values[i];
				}
			}
			throw new IllegalArgumentException( Integer.toString(id) );
		}

	}

	public enum SendCommand {
		
		SEND_START( (short)1 ),
		
		SEND_MOUSE_EVENT( (short)10 ),
		SEND_KEY_EVENT( (short)11 ),
		SEND_HARD_KEY_EVENT( (short)12 ),
		CHANGE_LCD_STATE( (short)13 ),
		OPEN_SHELL( (short)14 ),
		USB_KBD( (short)15 ),
		SCREEN_SHOT( (short)16 ),
		DETAIL_INFO( (short)17 ),
		RAM_DUMP( (short)18 ),
		GUEST_DUMP( (short)19 ),
		
		RESPONSE_HEART_BEAT( (short)900 ),
		CLOSE( (short)998 ),
		RESPONSE_SHUTDOWN( (short)999 );
		
		private short value;
		SendCommand( short value ) {
			this.value = value;
		}
		public short value() {
			return this.value;
		}
		public static SendCommand getValue( String val ) {
			SendCommand[] values = SendCommand.values();
			for (int i = 0; i < values.length; i++) {
				if( values[i].value == Short.parseShort( val ) ) {
					return values[i];
				}
			}
			throw new IllegalArgumentException( val );
		}
		public static SendCommand getValue( short val ) {
			SendCommand[] values = SendCommand.values();
			for (int i = 0; i < values.length; i++) {
				if( values[i].value == val ) {
					return values[i];
				}
			}
			throw new IllegalArgumentException( Integer.toString(val) );
		}
	}

	public enum ReceiveCommand {
		
		HEART_BEAT( (short)1 ),
		SCREEN_SHOT_DATA( (short)2 ),
		DETAIL_INFO_DATA( (short)3 ),
		RAMDUMP_COMPLETE( (short)4 ),
		SENSOR_DAEMON_START( (short)800 ),
		SHUTDOWN( (short)999 );
		
		private short value;
		ReceiveCommand( short value ) {
			this.value = value;
		}
		public short value() {
			return this.value;
		}
		public static ReceiveCommand getValue( String val ) {
			ReceiveCommand[] values = ReceiveCommand.values();
			for (int i = 0; i < values.length; i++) {
				if( values[i].value == Short.parseShort( val ) ) {
					return values[i];
				}
			}
			throw new IllegalArgumentException( val );
		}
		public static ReceiveCommand getValue( short val ) {
			ReceiveCommand[] values = ReceiveCommand.values();
			for (int i = 0; i < values.length; i++) {
				if( values[i].value == val ) {
					return values[i];
				}
			}
			throw new IllegalArgumentException( Integer.toString(val) );
		}
	}

	public void sendToQEMU( SendCommand command, ISendData data );
	
	public void terminate();
	
}
