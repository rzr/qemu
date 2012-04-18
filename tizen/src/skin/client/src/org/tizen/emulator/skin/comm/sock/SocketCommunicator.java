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

package org.tizen.emulator.skin.comm.sock;

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.tizen.emulator.skin.EmulatorSkin;
import org.tizen.emulator.skin.comm.ICommunicator;
import org.tizen.emulator.skin.comm.ICommunicator.SendCommand;
import org.tizen.emulator.skin.comm.sock.data.ISendData;
import org.tizen.emulator.skin.comm.sock.data.StartData;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.util.IOUtil;
import org.tizen.emulator.skin.util.SkinUtil;


/**
 * 
 * 
 */
public class SocketCommunicator implements ICommunicator {

	public class DataTranfer {

		private boolean isTransferState;
		private byte[] receivedData;

		private long sleep;
		private long maxWaitTime;
		private Timer timer;
		
		private DataTranfer() {
		}

		private void setData( byte[] data ) {
			this.receivedData = data;
			isTransferState = false;
		}

	}

	public static final int HEART_BEAT_INTERVAL = 1; //second
	public static final int HEART_BEAT_EXPIRE = 5;

	public final static int SCREENSHOT_WAIT_INTERVAL = 3; // milli-seconds
	public final static int SCREENSHOT_WAIT_LIMIT = 3000; // milli-seconds
	public final static int DETAIL_INFO_WAIT_INTERVAL = 1; // milli-seconds
	public final static int DETAIL_INFO_WAIT_LIMIT = 3000; // milli-seconds
	
	private static int reqId;
	
	private Logger logger = SkinLogger.getSkinLogger( SocketCommunicator.class ).getLogger();

	private EmulatorConfig config;
	private int uId;
	private int windowHandleId;
	private EmulatorSkin skin;

	private Socket socket;
	private DataInputStream dis;
	private DataOutputStream dos;

	private AtomicInteger heartbeatCount;
	private boolean isTerminated;
	private boolean isSensorDaemonStarted;
	private ScheduledExecutorService heartbeatExecutor;

	private DataTranfer screenShotDataTransfer;
	private DataTranfer detailInfoTransfer;
	
	private Thread sendThread;
	private ConcurrentLinkedQueue<SkinSendData> sendQueue;

	public SocketCommunicator( EmulatorConfig config, int uId, int windowHandleId, EmulatorSkin skin ) {

		this.config = config;
		this.uId = uId;
		this.windowHandleId = windowHandleId;
		this.skin = skin;

		this.screenShotDataTransfer = new DataTranfer();
		this.screenShotDataTransfer.sleep = SCREENSHOT_WAIT_INTERVAL;
		this.screenShotDataTransfer.maxWaitTime = SCREENSHOT_WAIT_LIMIT;

		this.detailInfoTransfer = new DataTranfer();
		this.detailInfoTransfer.sleep = DETAIL_INFO_WAIT_INTERVAL;
		this.detailInfoTransfer.maxWaitTime = DETAIL_INFO_WAIT_LIMIT;

		this.heartbeatCount = new AtomicInteger( 0 );
		this.heartbeatExecutor = Executors.newSingleThreadScheduledExecutor();

		try {

			int port = config.getArgInt( ArgsConstants.SERVER_PORT );
			socket = new Socket( "127.0.0.1", port );
			logger.info( "socket.isConnected():" + socket.isConnected() );

		} catch ( UnknownHostException e ) {
			logger.log( Level.SEVERE, e.getMessage(), e );
		} catch ( IOException e ) {
			logger.log( Level.SEVERE, e.getMessage(), e );
		}

	}

	@Override
	public void run() {

		try {

			sendQueue = new ConcurrentLinkedQueue<SkinSendData>();

			sendThread = new Thread() {
				@Override
				public void run() {

					while ( true ) {

						synchronized ( sendThread ) {
							try {
								sendThread.wait();
							} catch ( InterruptedException e ) {
								logger.log( Level.SEVERE, e.getMessage(), e );
							}
						}

						SkinSendData sendData = null;
						while ( true ) {
							sendData = sendQueue.poll();
							if ( null != sendData ) {
								sendToQEMUInternal( sendData );
							} else {
								break;
							}
						}

						if ( isTerminated ) {
							break;
						}

					}

				}
			};

			sendThread.start();

			dis = new DataInputStream( socket.getInputStream() );
			dos = new DataOutputStream( socket.getOutputStream() );

			int width = config.getArgInt( ArgsConstants.RESOLUTION_WIDTH );
			int height = config.getArgInt( ArgsConstants.RESOLUTION_HEIGHT );
			int scale = SkinUtil.getValidScale( config );
//			short rotation = config.getSkinPropertyShort( SkinPropertiesConstants.WINDOW_ROTATION,
//					EmulatorConfig.DEFAULT_WINDOW_ROTATION );
			// has to be portrait mode at first booting time
			short rotation = EmulatorConfig.DEFAULT_WINDOW_ROTATION;
			StartData startData = new StartData( windowHandleId, width, height, scale, rotation );

			sendToQEMU( SendCommand.SEND_START, startData );

		} catch ( IOException e ) {
			logger.log( Level.SEVERE, e.getMessage(), e );
			terminate();
			return;
		}

		boolean ignoreHeartbeat = config.getArgBoolean( ArgsConstants.TEST_HEART_BEAT_IGNORE );

		if ( ignoreHeartbeat ) {
			logger.info( "Ignore Skin heartbeat." );
		} else {

			heartbeatExecutor.scheduleAtFixedRate( new Runnable() {

				@Override
				public void run() {

					increaseHeartbeatCount();

					if ( isHeartbeatExpired() ) {
						terminate();
					}

				}

			}, 0, HEART_BEAT_INTERVAL, TimeUnit.SECONDS );

		}

		while ( true ) {

			if ( isTerminated ) {
				break;
			}

			try {

				int reqId = dis.readInt();
				short cmd = dis.readShort();
				int length = dis.readInt();

				if ( logger.isLoggable( Level.FINE ) ) {
					logger.fine( "[Socket] read - reqId:" + reqId + ", command:" + cmd + ", dataLength:" + length );
				}

				ReceiveCommand command = null;

				try {
					command = ReceiveCommand.getValue( cmd );
				} catch ( IllegalArgumentException e ) {
					logger.severe( "unknown command:" + cmd );
					continue;
				}

				switch ( command ) {
				case HEART_BEAT: {
					resetHeartbeatCount();
					if ( logger.isLoggable( Level.FINE ) ) {
						logger.fine( "received HEAR_BEAT from QEMU." );
					}
					sendToQEMU( SendCommand.RESPONSE_HEART_BEAT, null );
					break;
				}
				case SCREEN_SHOT_DATA: {
					logger.info( "received SCREEN_SHOT_DATA from QEMU." );
					receiveData( screenShotDataTransfer, length );

					break;
				}
				case DETAIL_INFO_DATA: {
					logger.info( "received DETAIL_INFO_DATA from QEMU." );
					receiveData( detailInfoTransfer, length );

					break;
				}
				case SENSOR_DAEMON_START: {
					logger.info( "received SENSOR_DAEMON_START from QEMU." );
					synchronized ( this ) {
						isSensorDaemonStarted = true;
					}
					break;
				}
				case SHUTDOWN: {
					logger.info( "received RESPONSE_SHUTDOWN from QEMU." );
					sendToQEMU( SendCommand.RESPONSE_SHUTDOWN, null );
					terminate();
					break;
				}
				default: {
					logger.severe( "Unknown command from QEMU. command:" + cmd );
					break;
				}
				}

			} catch ( IOException e ) {
				logger.log( Level.SEVERE, e.getMessage(), e );
				break;
			}

		}

	}

	private void receiveData( DataTranfer dataTransfer, int length ) throws IOException {
		
		synchronized ( dataTransfer ) {
			
			if( null != dataTransfer.timer ) {
				dataTransfer.timer.cancel();
			}
			
			byte[] data = readData( dis, length );
			
			if( null != data ) {
				logger.info( "finished receiving data from QEMU." );
			}else {
				logger.severe( "Fail to receiving data from QEMU." );
			}
			
			dataTransfer.isTransferState = false;
			dataTransfer.timer = null;
			
			dataTransfer.setData( data );
			dataTransfer.notifyAll();
			
		}

	}

	private byte[] readData( DataInputStream is, int length ) throws IOException {

		if ( 0 >= length ) {
			return null;
		}

		BufferedInputStream bfis = new BufferedInputStream( is, length );
		byte[] data = new byte[length];

		int read = 0;
		int total = 0;

		while ( true ) {

			if ( total == length ) {
				break;
			}

			read = bfis.read( data, total, length - total );

			if ( 0 > read ) {
				if ( total < length ) {
					continue;
				}
			} else {
				total += read;
			}

		}

		logger.info( "finished reading stream. read:" + total );

		return data;

	}

	public synchronized DataTranfer sendToQEMU( SendCommand command, ISendData data, boolean useDataTransfer ) {

		DataTranfer dataTranfer = null;
		
		if ( useDataTransfer ) {

			if ( SendCommand.SCREEN_SHOT.equals( command ) ) {
				dataTranfer = resetDataTransfer( screenShotDataTransfer );
			} else if ( SendCommand.DETAIL_INFO.equals( command ) ) {
				dataTranfer = resetDataTransfer( detailInfoTransfer );
			}
		}

		sendToQEMU( command, data );
		
		return dataTranfer;

	}
	
	private DataTranfer resetDataTransfer( final DataTranfer dataTransfer ) {
		
		synchronized ( dataTransfer ) {
			
			if ( dataTransfer.isTransferState ) {
				logger.severe( "Already transter state for getting data." );
				return null;
			}

			dataTransfer.isTransferState = true;

			Timer timer = new Timer();
			dataTransfer.timer = timer;

			TimerTask timerTask = new TimerTask() {
				@Override
				public void run() {
					synchronized ( dataTransfer ) {
						dataTransfer.isTransferState = false;
						dataTransfer.timer = null;
						dataTransfer.receivedData = null;
					}
				}
			};
			timer.schedule( timerTask, dataTransfer.maxWaitTime );

			return dataTransfer;

		}

	}
	
	@Override
	public void sendToQEMU( SendCommand command, ISendData data ) {
		
		sendQueue.add( new SkinSendData( command, data ) );
		
		synchronized ( sendThread ) {
			sendThread.notifyAll();
		}

	}
	
	private void sendToQEMUInternal( SkinSendData sendData ) {

		try {

			if( null == sendData ) {
				return;
			}
			
			SendCommand command = sendData.getCommand();
			ISendData data = sendData.getSendData();
			
			reqId = ( Integer.MAX_VALUE == reqId ) ? 0 : ++reqId;
			
			ByteArrayOutputStream bao = new ByteArrayOutputStream();
			DataOutputStream dataOutputStream = new DataOutputStream( bao );

			dataOutputStream.writeInt( uId );
			dataOutputStream.writeInt( reqId );
			dataOutputStream.writeShort( command.value() );

			short length = 0;
			if ( null == data ) {
				length = 0;
				dataOutputStream.writeShort( length );
			} else {
				byte[] byteData = data.serialize();
				length = (short) byteData.length;
				dataOutputStream.writeShort( length );
				dataOutputStream.write( byteData );
			}

			dataOutputStream.flush();

			dos.write( bao.toByteArray() );
			dos.flush();

			if ( logger.isLoggable( Level.FINE ) ) {
				logger.fine( "[Socket] write - uid:" + uId + ", reqId:" + reqId + ", command:" + command.value()
						+ " - " + command.toString() + ", length:" + length );
			}

			if ( 0 < length ) {
				if ( logger.isLoggable( Level.FINE ) ) {
					logger.fine( "[Socket] data  - " + data.toString() );
				}
			}

		} catch ( IOException e ) {
			logger.log( Level.SEVERE, e.getMessage(), e );
		}

	}

	public byte[] getReceivedData( DataTranfer dataTranfer ) {

		if( null == dataTranfer ) {
			return null;
		}
		
		synchronized ( dataTranfer ) {
			
			int count = 0;
			byte[] receivedData = null;
			long sleep = dataTranfer.sleep;
			long maxWaitTime = dataTranfer.maxWaitTime;
			int limitCount = (int) ( maxWaitTime / sleep );

			while ( dataTranfer.isTransferState ) {

				if ( limitCount < count ) {
					logger.severe( "time out for receiving data from skin server." );
					dataTranfer.receivedData = null;
					break;
				}

				try {
					dataTranfer.wait( sleep );
				} catch ( InterruptedException e ) {
					logger.log( Level.SEVERE, e.getMessage(), e );
				}

				count++;
				logger.info( "wait data... count:" + count );

			}

			receivedData = dataTranfer.receivedData;
			dataTranfer.receivedData = null;
			
			return receivedData;

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
		if ( logger.isLoggable( Level.FINE ) ) {
			logger.fine( "HB count : " + count );
		}
	}

	private boolean isHeartbeatExpired() {
		return HEART_BEAT_EXPIRE < heartbeatCount.get();
	}

	private void resetHeartbeatCount() {
		heartbeatCount.set( 0 );
	}

	@Override
	public void terminate() {
		
		isTerminated = true;
		
		if ( null != heartbeatExecutor ) {
			heartbeatExecutor.shutdownNow();
		}
		
		if( null != sendThread ) {
			synchronized ( sendThread ) {
				sendThread.notifyAll();
			}
		}
		
		IOUtil.closeSocket( socket );
		
		synchronized ( this ) {
			skin.shutdown();
		}
		
	}

	public void resetSkin( EmulatorSkin skin ) {
		synchronized ( this ) {
			this.skin = skin;
		}
	}

}

class SkinSendData {

	private SendCommand command;
	private ISendData data;

	public SkinSendData( SendCommand command, ISendData data ) {
		this.command = command;
		this.data = data;
	}

	public SendCommand getCommand() {
		return command;
	}

	public ISendData getSendData() {
		return data;
	}

}