/**
 * Communicate With Qemu
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

package org.tizen.emulator.skin.comm.sock;

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.MenuItem;
import org.tizen.emulator.skin.EmulatorSkin;
import org.tizen.emulator.skin.comm.ICommunicator;
import org.tizen.emulator.skin.comm.ICommunicator.SendCommand;
import org.tizen.emulator.skin.comm.sock.data.BooleanData;
import org.tizen.emulator.skin.comm.sock.data.ISendData;
import org.tizen.emulator.skin.comm.sock.data.StartData;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.config.EmulatorConfig.SkinPropertiesConstants;
import org.tizen.emulator.skin.image.ImageRegistry.ResourceImageName;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.util.IOUtil;


/**
 * 
 * 
 */
public class SocketCommunicator implements ICommunicator {
	public static class DataTranfer {
		private boolean isTransferState;
		private byte[] receivedData;

		private long sleep;
		private long maxWaitTime;
		private Timer timer;
		
		private DataTranfer() {
			/* do nothing */
		}

		private void setData(byte[] data) {
			this.receivedData = data;
			isTransferState = false;
		}

	}

	public static final int HEART_BEAT_INTERVAL = 1; /* seconds */
	public static final int HEART_BEAT_EXPIRE = 15;

	public final static int LONG_WAIT_INTERVAL = 3; /* ms */
	public final static int LONG_WAIT_LIMIT = 3000; /* ms */
	public final static int SHORT_WAIT_INTERVAL = 1;
	public final static int SHORT_WAIT_LIMIT = 2000;

	public final static int MAX_SEND_QUEUE_SIZE = 100000;

	private static int reqId;

	private Logger logger =
			SkinLogger.getSkinLogger(SocketCommunicator.class).getLogger();

	private EmulatorConfig config;
	private int uId;
	private StartData startData;
	private EmulatorSkin skin;

	private Socket socket;
	private DataInputStream sockInputStream;
	private DataOutputStream sockOutputStream;

	private AtomicInteger heartbeatCount;
	private boolean isTerminated;
	private boolean isSensorDaemonStarted;
	private boolean isSdbDaemonStarted;
	private boolean isEcsServerStarted;
	private boolean isRamdump;
	private TimerTask heartbeatExecutor;
	private Timer heartbeatTimer;

	private DataTranfer screenShotDataTransfer;
	private DataTranfer detailInfoTransfer;
	private DataTranfer miscDataTransfer;
	private DataTranfer ecpTransfer;

	private Thread sendThread;
	private LinkedList<SkinSendData> sendQueue;

	private ByteArrayOutputStream bao;
	private DataOutputStream dataOutputStream;

	/**
	 *  Constructor
	 */
	public SocketCommunicator(EmulatorConfig config, int uId, EmulatorSkin skin) {
		this.config = config;
		this.uId = uId;
		this.skin = skin;

		this.screenShotDataTransfer = new DataTranfer();
		this.screenShotDataTransfer.sleep = LONG_WAIT_INTERVAL;
		this.screenShotDataTransfer.maxWaitTime = LONG_WAIT_LIMIT;

		this.detailInfoTransfer = new DataTranfer();
		this.detailInfoTransfer.sleep = SHORT_WAIT_INTERVAL;
		this.detailInfoTransfer.maxWaitTime = SHORT_WAIT_LIMIT;

		this.miscDataTransfer = new DataTranfer();
		this.miscDataTransfer.sleep = SHORT_WAIT_INTERVAL;
		this.miscDataTransfer.maxWaitTime = SHORT_WAIT_LIMIT;

		this.ecpTransfer = new DataTranfer();
		this.ecpTransfer.sleep = LONG_WAIT_INTERVAL;
		this.ecpTransfer.maxWaitTime = LONG_WAIT_LIMIT;

		this.heartbeatCount = new AtomicInteger(0);
		//this.heartbeatExecutor = Executors.newSingleThreadScheduledExecutor();
		this.heartbeatTimer = new Timer();

		try {
			int port = config.getArgInt(ArgsConstants.VM_SKIN_PORT);
			socket = new Socket("127.0.0.1", port);
			logger.info("socket.isConnected() : " + socket.isConnected());
		} catch (UnknownHostException e) {
			logger.log(Level.SEVERE, e.getMessage(), e);
		} catch (IOException e) {
			logger.log(Level.SEVERE, e.getMessage(), e);
		}

		this.bao = new ByteArrayOutputStream();
		this.dataOutputStream = new DataOutputStream(bao);
	}

	public void setInitialData(StartData data) {
		this.startData = data;
	}

	private void sendInitialData() {
		logger.info("send startData");
		sendToQEMU(SendCommand.SEND_SKIN_OPENED, startData, false);

		/* default interpolation */
		skin.getShell().getDisplay().asyncExec(new Runnable() {
			@Override
			public void run() {
				skin.isOnInterpolation = Boolean.parseBoolean(config.getSkinProperty(
						SkinPropertiesConstants.WINDOW_INTERPOLATION,
						Boolean.TRUE.toString()));

				MenuItem item = skin.getPopupMenu().interpolationHighItem;
				if (item != null) {
					item.setSelection(skin.isOnInterpolation);
				}

				item = skin.getPopupMenu().interpolationLowItem;
				if (item != null) {
					item.setSelection(!skin.isOnInterpolation);
				}

				BooleanData dataInterpolation = new BooleanData(
						skin.isOnInterpolation,
						SendCommand.SEND_INTERPOLATION_STATE.toString());
				sendToQEMU(SendCommand.SEND_INTERPOLATION_STATE, dataInterpolation, false);
			}
		});
	}

	@Override
	public void run() {
		sendQueue = new LinkedList<SkinSendData>();

		sendThread = new Thread("sendThread") {
			List<SkinSendData> list = new ArrayList<SkinSendData>();

			@Override
			public void run() {
				while (true) {
					synchronized (sendQueue) {
						if (sendQueue.isEmpty()) {
							try {
								sendQueue.wait();
							} catch (InterruptedException e) {
								logger.log(Level.SEVERE, e.getMessage(), e);
							}
						}

						SkinSendData sendData = null;
						while (true) {
							sendData = sendQueue.poll();
							if (null != sendData) {
								list.add(sendData);
							} else {
								break;
							}
						}
					}

					if (isTerminated) {
						list.clear();
						break;
					}

					for (SkinSendData data : list) {
						sendToQEMUInternal(data);
					}

					list.clear();
				}
			}
		};

		try {
			sockInputStream = new DataInputStream(socket.getInputStream());
			sockOutputStream = new DataOutputStream(socket.getOutputStream());
		} catch (IOException e) {
			logger.log(Level.SEVERE, e.getMessage(), e);
			terminate();
			return;
		}

		sendThread.setDaemon(true);
		sendThread.start();

		/* notify to Qemu */
		sendInitialData();

		boolean ignoreHeartbeat =
				config.getArgBoolean(ArgsConstants.HEART_BEAT_IGNORE);

		if (ignoreHeartbeat) {
			logger.info("Ignore Skin heartbeat.");
		} else {
			heartbeatExecutor = new TimerTask() {
				@Override
				public void run() {
					increaseHeartbeatCount();

					if (isHeartbeatExpired()) {
						logger.info("heartbeat was expired!");
						terminate();
					}
				}
			};

			heartbeatTimer.schedule(heartbeatExecutor, 1, HEART_BEAT_INTERVAL * 1000);
		}

		while (true) {
			if (isTerminated) {
				break;
			}

			try {
				int reqId = sockInputStream.readInt();
				short cmd = sockInputStream.readShort();
				int length = sockInputStream.readInt();

				if (logger.isLoggable(Level.FINE)) {
					logger.fine("[Socket] read - reqId:" + reqId +
							", command:" + cmd + ", dataLength:" + length);
				}

				ReceiveCommand command = null;

				try {
					command = ReceiveCommand.getValue(cmd);
				} catch (IllegalArgumentException e) {
					logger.severe("unknown command:" + cmd);
					continue;
				}

				switch (command) {
				case RECV_HEART_BEAT: {
					resetHeartbeatCount();

					if (logger.isLoggable(Level.FINE)) {
						logger.fine("received HEART_BEAT from QEMU");
					}

					sendToQEMU(SendCommand.RESPONSE_HEART_BEAT, null, true);
					break;
				}
				case RECV_SCREENSHOT_DATA: {
					logger.info("received SCREENSHOT_DATA from QEMU");
					receiveData(screenShotDataTransfer, length);

					break;
				}
				case RECV_DETAIL_INFO_DATA: {
					logger.info("received DETAIL_INFO_DATA from QEMU");
					receiveData(detailInfoTransfer, length);

					break;
				}
				case RECV_RAMDUMP_COMPLETED: {
					logger.info("received RAMDUMP_COMPLETED from QEMU");
					setRamdumpFlag(false);
					break;
				}
				case RECV_BOOTING_PROGRESS: {
					//logger.info("received BOOTING_PROGRESS from QEMU");

					resetDataTransfer(miscDataTransfer);
					receiveData(miscDataTransfer, length);

					byte[] receivedData = getReceivedData(miscDataTransfer);
					if (null != receivedData) {
						String strLayer = new String(receivedData, 0, 1, "UTF-8");
						String strValue = new String(receivedData, 1, length - 2, "UTF-8");

						int layer = 0;
						int value = 0;
						try {
							layer = Integer.parseInt(strLayer);
							value = Integer.parseInt(strValue);
						} catch (NumberFormatException e) {
							e.printStackTrace();
						}

						/* draw progress bar */
						skin.updateProgressBar(layer, value);
					}

					break;
				}
				case RECV_BRIGHTNESS_STATE: {
					//logger.info("received BRIGHTNESS_STATE from QEMU");

					resetDataTransfer(miscDataTransfer);
					receiveData(miscDataTransfer, length);

					byte[] receivedData = getReceivedData(miscDataTransfer);
					if (null != receivedData) {
						String strValue = new String(receivedData, 0, length - 1, "UTF-8");

						int value = 1;
						try {
							value = Integer.parseInt(strValue);
						} catch (NumberFormatException e) {
							e.printStackTrace();
						}

						if (value == 0) {
							skin.updateBrightness(false);
						} else {
							skin.updateBrightness(true);
						}
					}

					break;
				}
				case RECV_ECP_PORT_DATA: {
					logger.info("received ECP_PORT_DATA from QEMU");
					receiveData(ecpTransfer, length);

					break;
				}
				case RECV_HOST_KBD_STATE: {
					logger.info("received HOST_KBD_STATE from QEMU");

					resetDataTransfer(miscDataTransfer);
					receiveData(miscDataTransfer, length);

					byte[] receivedData = getReceivedData(miscDataTransfer);
					if (null != receivedData) {
						String strValue = new String(receivedData, 0, length - 1, "UTF-8");

						int value = 1;
						try {
							value = Integer.parseInt(strValue);
						} catch (NumberFormatException e) {
							e.printStackTrace();
						}

						if (value == 0) {
							skin.updateHostKbdMenu(false);
						} else {
							skin.updateHostKbdMenu(true);
						}
					}

					break;
				}
				case RECV_SENSORD_STARTED: {
					logger.info("received SENSORD_STARTED from QEMU");

					synchronized (this) {
						isSensorDaemonStarted = true;
					}
					break;
				}
				case RECV_SDBD_STARTED: {
					logger.info("received SDBD_STARTED from QEMU");

					synchronized (this) {
						isSdbDaemonStarted = true;
					}
					break;
				}
				case RECV_ECS_STARTED: {
					logger.info("received ECS_STARTED from QEMU");

					synchronized (this) {
						isEcsServerStarted = true;
					}
					break;
				}
				case RECV_DRAW_FRAME: {
					//logger.info("received DRAW_FRAME from QEMU.");

					skin.updateDisplay();

					break;
				}
				case RECV_DRAW_BLANK_GUIDE: {
					logger.info("received DRAW_BLANK_GUIDE from QEMU.");

					Image imageGuide = skin.getImageRegistry().getResourceImage(
							ResourceImageName.RESOURCE_BLANK_GUIDE);
					if (imageGuide != null) {
						skin.drawImageToDisplay(imageGuide);
					}

					break;
				}
				case RECV_EMUL_RESET: {
					logger.info("received EMUL_RESET from QEMU");

					isSensorDaemonStarted = false;
					isSdbDaemonStarted = false;
					isEcsServerStarted = false;
					// TODO:

					break;
				}
				case RECV_EMUL_SHUTDOWN: {
					logger.info("received EMUL_SHUTDOWN from QEMU");

					sendToQEMU(SendCommand.RESPONSE_SHUTDOWN, null, false);
					terminate();

					break;
				}
				default: {
					logger.severe("Unknown command from QEMU. command : " + cmd);
					break;
				}
				}
			} catch (IOException e) {
				logger.log(Level.SEVERE, e.getMessage(), e);
				break;
			}

		}

		logger.info("communicatorThread is stopped");
	}

	private void receiveData(
			DataTranfer dataTransfer, int length) throws IOException {
		synchronized (dataTransfer) {

			if (null != dataTransfer.timer) {
				dataTransfer.timer.cancel();
			}

			byte[] data = readData(sockInputStream, length);

			if (null != data) {
				logger.info("finished receiving data from QEMU.");
			} else {
				logger.severe("Fail to receiving data from QEMU.");
			}

			dataTransfer.isTransferState = false;
			dataTransfer.timer = null;
			
			dataTransfer.setData(data);
			dataTransfer.notifyAll();
		}
	}

	private byte[] readData(
			DataInputStream is, int length) throws IOException {
		if (0 >= length) {
			return null;
		}

		BufferedInputStream bfis = new BufferedInputStream(is, length);
		byte[] data = new byte[length];
		int read = 0;
		int total = 0;

		while (true) {
			if (total == length) {
				break;
			}

			read = bfis.read(data, total, length - total);

			if (0 > read) {
				if (total < length) {
					continue;
				}
			} else {
				total += read;
			}
		}

		logger.info("finished reading stream. read : " + total);

		return data;
	}

	private DataTranfer resetDataTransfer(final DataTranfer dataTransfer) {
		synchronized(dataTransfer) {
			if (dataTransfer.isTransferState) {
				logger.severe("Already transter state for getting data.");
				return null;
			}

			dataTransfer.isTransferState = true;

			Timer timer = new Timer();
			dataTransfer.timer = timer;

			TimerTask timerTask = new TimerTask() {
				@Override
				public void run() {
					synchronized(dataTransfer) {
						dataTransfer.isTransferState = false;
						dataTransfer.timer = null;
						dataTransfer.receivedData = null;
					}
				}
			};
			timer.schedule(timerTask, dataTransfer.maxWaitTime + 1000);

			return dataTransfer;
		}
	}

	public synchronized DataTranfer sendDataToQEMU(
			SendCommand command, ISendData data, boolean useDataTransfer) {
		DataTranfer dataTranfer = null;

		if (useDataTransfer) {
			if (SendCommand.SEND_SCREENSHOT_REQ.equals(command)) {
				dataTranfer = resetDataTransfer(screenShotDataTransfer);
			} else if (SendCommand.SEND_DETAIL_INFO_REQ.equals(command)) {
				dataTranfer = resetDataTransfer(detailInfoTransfer);
			} else if (SendCommand.SEND_ECP_PORT_REQ.equals(command)) {
				dataTranfer = resetDataTransfer(ecpTransfer);
			}
		}

		sendToQEMU(command, data, false);

		return dataTranfer;
	}

	@Override
	public void sendToQEMU(SendCommand command, ISendData data, boolean urgent) {
		synchronized(sendQueue) {
			if (MAX_SEND_QUEUE_SIZE < sendQueue.size()) {
				logger.warning(
						"Send queue size exceeded max value, do not push data into send queue.");
			} else {
				if (urgent == true) {
					sendQueue.addFirst(new SkinSendData(command, data));
				} else {
					sendQueue.add(new SkinSendData(command, data));
				}

				sendQueue.notifyAll();
			}
		}
	}
	
	private void sendToQEMUInternal(SkinSendData sendData) {
		if (null == sendData) {
			return;
		}

		SendCommand command = sendData.getCommand();
		ISendData data = sendData.getSendData();

		reqId = (Integer.MAX_VALUE == reqId) ? 0 : ++reqId;

		try {
			dataOutputStream.writeInt(uId);
			dataOutputStream.writeInt(reqId);
			dataOutputStream.writeShort(command.value());

			short length = 0;
			if (null == data) {
				length = 0;
				dataOutputStream.writeShort(length);
			} else {
				byte[] byteData = data.serialize();
				length = (short) byteData.length;
				dataOutputStream.writeShort(length);
				dataOutputStream.write(byteData);
			}

			dataOutputStream.flush();

			sockOutputStream.write(bao.toByteArray());
			sockOutputStream.flush();

			bao.reset();

			if (logger.isLoggable(Level.FINE)) {
				logger.fine("[Socket] write - uid:" + uId +
						", reqId:" + reqId + ", command:" + command.value()
						+ " - " + command.toString() + ", length:" + length);
			}

			if (0 < length) {
				if (logger.isLoggable(Level.FINE)) {
					logger.fine("[Socket] data  - " + data.toString());
				}
			}
		} catch (IOException e) {
			logger.log(Level.SEVERE, e.getMessage(), e);
		}
	}

	public byte[] getReceivedData(DataTranfer dataTranfer) {
		if (null == dataTranfer) {
			return null;
		}

		synchronized (dataTranfer) {
			int count = 0;
			byte[] receivedData = null;
			long sleep = dataTranfer.sleep;
			long maxWaitTime = dataTranfer.maxWaitTime;
			int limitCount = (int) (maxWaitTime / sleep);

			while (dataTranfer.isTransferState) {
				if (limitCount < count) {
					logger.severe("time out for receiving data from skin server.");

					dataTranfer.receivedData = null;
					break;
				}

				try {
					dataTranfer.wait(sleep);
				} catch (InterruptedException e) {
					logger.log(Level.SEVERE, e.getMessage(), e);
				}

				count++;
				logger.info("wait data... count : " + count);
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

	public synchronized boolean isSdbDaemonStarted() {
		return isSdbDaemonStarted;
	}

	public synchronized boolean isEcsServerStarted() {
		return isEcsServerStarted;
	}

	public synchronized void setRamdumpFlag(boolean flag) {
		isRamdump = flag;
	}

	public synchronized boolean getRamdumpFlag() {
		return isRamdump;
	}

	private void increaseHeartbeatCount() {
		int count = heartbeatCount.incrementAndGet();

		if (count > 1) {
			logger.info("HB count : " + count);
		}
	}

	private boolean isHeartbeatExpired() {
		return HEART_BEAT_EXPIRE < heartbeatCount.get();
	}

	private void resetHeartbeatCount() {
		heartbeatCount.set(0);

		if (logger.isLoggable(Level.FINE)) {
			logger.info("HB count reset");
		}
	}

	@Override
	public void terminate() {
		if (isTerminated == true) {
			logger.info("has been terminated");
			return;
		}

		logger.info("terminated");

		isTerminated = true;

		synchronized (sendQueue) {
			sendQueue.notifyAll();
		}

		if (null != heartbeatTimer) {
			heartbeatTimer.cancel();
			heartbeatTimer = null;
		}

		IOUtil.closeSocket(socket);
		IOUtil.close(sockInputStream);
		IOUtil.close(sockOutputStream);

		IOUtil.close(bao);
		IOUtil.close(dataOutputStream);

		synchronized (this) {
			skin.shutdown();
		}
	}
}

class SkinSendData {
	private SendCommand command;
	private ISendData data;

	public SkinSendData(SendCommand command, ISendData data) {
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
