/**
 * 
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Hyunjun Son <hj79.son@samsung.com>
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

package org.tizen.emulator.skin.comm.sock;

import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.UUID;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

import org.tizen.emulator.skin.EmulatorConstants;
import org.tizen.emulator.skin.EmulatorSkin;
import org.tizen.emulator.skin.comm.ICommunicator;
import org.tizen.emulator.skin.comm.sock.data.ISendData;
import org.tizen.emulator.skin.comm.sock.data.StartData;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.config.EmulatorConfig.PropertiesConstants;
import org.tizen.emulator.skin.util.IOUtil;


/**
 * 
 * 
 */
public class SocketCommunicator implements ICommunicator {

	private EmulatorConfig config;
	private int uId;
	private int windowHandleId;
	private EmulatorSkin skin;

	private Socket socket;
	private DataInputStream dis;
	private DataOutputStream dos;

	private AtomicInteger heartbeatCount;

	private boolean isTerminated;
	private ScheduledExecutorService heartbeatExecutor;
	
	private boolean isSensorDaemonStarted;

	public SocketCommunicator(EmulatorConfig config, int uId, int windowHandleId, EmulatorSkin skin) {

		this.config = config;
		this.uId = uId;
		this.windowHandleId = windowHandleId;
		this.skin = skin;

		this.heartbeatCount = new AtomicInteger(0);
		this.heartbeatExecutor = Executors.newSingleThreadScheduledExecutor();

		try {

			String portString = config.getArg(ArgsConstants.SERVER_PORT);
			int port = Integer.parseInt(portString);
			socket = new Socket("127.0.0.1", port);
			System.out.println("socket.isConnected():" + socket.isConnected());

		} catch (UnknownHostException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}

	}

	@Override
	public void run() {

		try {

			dis = new DataInputStream(socket.getInputStream());
			dos = new DataOutputStream(socket.getOutputStream());
			
			short scale = config.getPropertyShort(PropertiesConstants.WINDOW_SCALE, Scale.HALF.value() );
			short rotation = config.getPropertyShort( PropertiesConstants.WINDOW_DIRECTION, (short) 0 );
			
			sendToQEMU(SendCommand.SEND_START,
					new StartData(windowHandleId,
							Integer.parseInt( config.getArg(ArgsConstants.RESOLUTION_WIDTH) ),
							Integer.parseInt( config.getArg(ArgsConstants.RESOLUTION_HEIGHT) ),
							scale, rotation));

		} catch (IOException e) {
			e.printStackTrace();
			terminate();
			return;
		}
		
		String ignoreHeartbeatString = config.getArg( ArgsConstants.TEST_HEART_BEAT_IGNORE, Boolean.FALSE.toString() );
		Boolean ignoreHeartbeat = Boolean.parseBoolean( ignoreHeartbeatString );
		
		if( ignoreHeartbeat ) {
			System.out.println( "Ignore Skin heartbeat." );
		}else {
			
			heartbeatExecutor.scheduleAtFixedRate(new Runnable() {
				
				@Override
				public void run() {
					
					increaseHeartbeatCount();
					
					if (isHeartbeatExpired()) {
						terminate();
					}
					
				}
				
			}, 0, EmulatorConstants.HEART_BEAT_INTERVAL, TimeUnit.SECONDS);
			
		}

		while ( true ) {

			if ( isTerminated ) {
				break;
			}

			try {

				int reqId = dis.readInt();
				short cmd = dis.readShort();

				System.out.print( "= Socket read - reqId:" + reqId + ", command:" + cmd + ", " );

				ReceiveCommand command = null;
				
				try {
					command = ReceiveCommand.getValue( cmd );
				} catch ( IllegalArgumentException e ) {
					System.out.println( "unknown command:" + cmd );
					continue;
				}

				switch ( command ) {
				case HEART_BEAT:
					resetHeartbeatCount();
					System.out.println( "received HEAR_BEAT from QEMU." );
					sendToQEMU( SendCommand.RESPONSE_HEART_BEAT, null );
					break;
				case SENSOR_DAEMON_START:
					System.out.println( "received SENSOR_DAEMON_START from QEMU." );
					synchronized ( this ) {
						isSensorDaemonStarted = true;
					}
					break;
				case SHUTDOWN:
					sendToQEMU( SendCommand.RESPONSE_SHUTDOWN, null );
					isTerminated = true;
					terminate();
					break;
				default:
					break;
				}

			} catch ( IOException e ) {
				e.printStackTrace();
				break;
			}

		}
		
	}

	@Override
	public void sendToQEMU(SendCommand command, ISendData data) {

		try {

			//anyway down casting
			int reqId = (int) UUID.randomUUID().getMostSignificantBits();
			
			ByteArrayOutputStream bao = new ByteArrayOutputStream();
			DataOutputStream dataOutputStream = new DataOutputStream(bao);
			
			dataOutputStream.writeInt(uId);
			dataOutputStream.writeInt(reqId);
			dataOutputStream.writeShort(command.value());
			
			short length = 0;
			if( null == data ) {
				length = 0;
				dataOutputStream.writeShort(length);
			}else {
				byte[] byteData = data.serialize();
				length = (short) byteData.length;
				dataOutputStream.writeShort(length);
				dataOutputStream.write(byteData);
			}

			dataOutputStream.flush();
			
			dos.write(bao.toByteArray());
			dos.flush();

			System.out.println("= Socket write - uid:" + uId + ", reqId:" + reqId +
					", command:" + command.value() + " - " + command.toString() + ", length:" + length );

			if (0 < length) {
				System.out.println("== data ==");
				System.out.println(data);
			}

		} catch (IOException e) {
			System.err.println("Fail to write socket.");
			e.printStackTrace();
		}

	}

	public Socket getSocket() {
		return socket;
	}

	public synchronized boolean isSensorDaemonStarted() {
		return isSensorDaemonStarted;
	}

	private void increaseHeartbeatCount() {
		int count = heartbeatCount.incrementAndGet();
		System.out.println("HB count : " + count);
	}

	private boolean isHeartbeatExpired() {
		return EmulatorConstants.HEART_BEAT_EXPIRE < heartbeatCount.get();
	}

	private void resetHeartbeatCount() {
		heartbeatCount.set(0);
	}

	@Override
	public void terminate() {
		if (null != heartbeatExecutor) {
			heartbeatExecutor.shutdownNow();
		}
		IOUtil.closeSocket(socket);
		skin.shutdown();
	}

}
