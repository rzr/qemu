/**
 *
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Munkyu Im <munkyu.im@samsung.com>
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

package org.tizen.emulator.skin;

import java.util.ArrayList;
import java.util.logging.Logger;

import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.PaletteData;
import org.eclipse.swt.widgets.Display;
import org.tizen.emulator.skin.comm.ICommunicator.MouseButtonType;
import org.tizen.emulator.skin.comm.ICommunicator.MouseEventType;
import org.tizen.emulator.skin.comm.ICommunicator.RotationInfo;
import org.tizen.emulator.skin.comm.ICommunicator.SendCommand;
import org.tizen.emulator.skin.comm.sock.SocketCommunicator;
import org.tizen.emulator.skin.comm.sock.data.MouseEventData;
import org.tizen.emulator.skin.log.SkinLogger;

public class EmulatorFingers {
	private static final int MAX_FINGER_CNT = 10;
	private static final int RED_MASK = 0x0000FF00;
	private static final int GREEN_MASK = 0x00FF0000;
	private static final int BLUE_MASK = 0xFF000000;
	private static final int COLOR_DEPTH = 32;

	private Logger logger =
			SkinLogger.getSkinLogger(EmulatorFingers.class).getLogger();

	private int multiTouchEnable;
	private int maxTouchPoint;
	protected int fingerCnt;
	private int fingerCntMax;
	protected int fingerPointSize;
	protected int fingerPointSizeHalf;
	private Color fingerPointColor;
	private Color fingerPointOutlineColor;
	private int grabFingerID = 0;
	protected Image fingerSlotimage;
	protected ImageData imageData;
	protected FingerPoint fingerSlot;
	protected SocketCommunicator communicator;
	protected EmulatorSkin emulatorSkin;
	ArrayList<FingerPoint> FingerPointList;

	EmulatorSkinState currentState;

	EmulatorFingers(EmulatorSkinState currentState) {
		this.currentState = currentState;
		initMultiTouchState();
	}

	protected void setCommunicator(SocketCommunicator communicator) {
		 this.communicator = communicator;
	 }

	//private fingerPointSurface;
	protected class FingerPoint {
		int id;
		int originX;
		int originY;
		int x;
		int y;

		FingerPoint(int originX, int originY, int x, int y) {
			this.originX = originX;
			this.originY = -originY;
			this.x = x;
			this.y = y;
		}

		FingerPoint(int id, int originX, int originY, int x, int y) {
			this.id = id;
			this.originX = originX;
			this.originY = -originY;
			this.x = x;
			this.y = y;
		}
	}

	public FingerPoint getFingerPointFromSlot(int index) {
		if (index < 0 || index > this.fingerCntMax) {
			return null;
		}

		return FingerPointList.get(index);
	}

	public FingerPoint getFingerPointSearch(int x, int y) {
		int i;
		FingerPoint fingerPoint = null;
		int fingerRegion = (this.fingerPointSize) + 2;
		//logger.info("x: "+x+ "y: "+ y + " fingerRegion: " + fingerRegion);

		for (i = this.fingerCnt - 1; i >= 0; i--) {
			fingerPoint = getFingerPointFromSlot(i);

			if (fingerPoint != null) {
				if (x >= (fingerPoint.x - fingerRegion) &&
						x < (fingerPoint.x + fingerRegion) &&
						y >= (fingerPoint.y - fingerRegion) &&
						y < (fingerPoint.y + fingerRegion)) {
					//logger.info("return finger point:" + fingerPoint);
					return fingerPoint;
				}
			}
		}

		return null;
	}

	protected void setEmulatorSkin(EmulatorSkin emulatorSkin) {
		this.emulatorSkin = emulatorSkin;
	}

	public void initMultiTouchState() {
		this.multiTouchEnable = 0;
		this.fingerCntMax = this.currentState.getMaxTouchPoint();

		if (this.fingerCntMax > MAX_FINGER_CNT) {
			this.fingerCntMax = MAX_FINGER_CNT;
			setMaxTouchPoint(this.fingerCntMax);
		}

		logger.info("maxTouchPoint="+ this.fingerCntMax);
		this.fingerCnt = 0;

		if (this.fingerSlot != null) {
			this.fingerSlot = null;
		}

		FingerPointList = new ArrayList<FingerPoint>();
		int i;
		for (i = 0; i <= fingerCntMax; i++) {
			FingerPointList.add(new FingerPoint(-1, -1, -1, -1));
		} 
		this.fingerPointSize = 32;
		this.fingerPointSizeHalf = this.fingerPointSize / 2;

		this.fingerPointOutlineColor = new Color(Display.getCurrent(), 0xDD, 0xDD, 0xDD);
		this.fingerPointColor = new Color(Display.getCurrent(), 0x0F, 0x0F, 0x0F);
		PaletteData palette = new PaletteData(RED_MASK, GREEN_MASK, BLUE_MASK);

		this.imageData = new ImageData(
				fingerPointSize + 4, fingerPointSize + 4, COLOR_DEPTH, palette);
		this.imageData.transparentPixel = 0;
		this.fingerSlotimage = new Image(Display.getCurrent(), imageData);

		GC gc = new GC(this.fingerSlotimage);

		gc.setForeground(this.fingerPointOutlineColor);
		gc.drawOval(0, 0, this.fingerPointSize + 2, this.fingerPointSize + 2);

		gc.setBackground(this.fingerPointColor);
		gc.fillOval(2, 2, this.fingerPointSize, this.fingerPointSize);

		gc.dispose();
	}

	public void setMultiTouchEnable(int multiTouchEnable) {
		this.multiTouchEnable = multiTouchEnable;
	}

	public int getMultiTouchEnable() {
		return this.multiTouchEnable;
	}

	protected int addFingerPoint(int originX, int originY, int x, int y) {
		if (this.fingerCnt == this.fingerCntMax) {
			logger.info("support multi-touch up to "
					+ this.fingerCntMax + " fingers");
			return -1;
		}
		this.fingerCnt += 1;

		FingerPointList.get(fingerCnt -1).id = this.fingerCnt;
		FingerPointList.get(fingerCnt -1).originX = originX;
		FingerPointList.get(fingerCnt -1).originY = originY;
		FingerPointList.get(fingerCnt -1).x = x;
		FingerPointList.get(fingerCnt -1).y = y;
		logger.info(this.fingerCnt + " finger touching");

		return this.fingerCnt;
	}

	protected void drawImage(PaintEvent e, int currentAngle) {
		//by mq
		for (int i = 0; i < this.fingerCnt; i++) {
			this.fingerSlot = this.getFingerPointFromSlot(i);	
			e.gc.setAlpha(0x7E);
		//	logger.info("OriginX: "+ this.fingerSlot.originX + ",OriginY: " + (this.fingerSlot.originY));
		//	logger.info("x: "+ this.fingerSlot.x + ",y: " + (this.fingerSlot.y));

			e.gc.drawImage(this.fingerSlotimage, 
					this.fingerSlot.originX - fingerPointSizeHalf - 2,
					this.fingerSlot.originY - fingerPointSizeHalf - 2);
			e.gc.setAlpha(0xFF);
		}
	}

	public void maruFingerProcessing1(
			int touchType, int originX, int originY, int x, int y) {
		FingerPoint finger = null;
		MouseEventData mouseEventData;

		if (touchType == MouseEventType.PRESS.value() ||
				touchType == MouseEventType.DRAG.value()) { /* pressed */
			if (grabFingerID > 0) {
				finger = getFingerPointFromSlot(grabFingerID - 1);
				if (finger != null) {
					finger.originX = originX;
					finger.originY = originY;
					finger.x = x;
					finger.y = y;

					if (finger.id != 0) {
						logger.info(String.format(
								"id %d finger multi-touch dragging = (%d, %d)",
								this.grabFingerID, x, y));

						mouseEventData = new MouseEventData(
								MouseButtonType.LEFT.value(), MouseEventType.PRESS.value(),
								originX, originY, x, y, grabFingerID - 1);
						communicator.sendToQEMU(SendCommand.SEND_MOUSE_EVENT, mouseEventData);
					} 
				}

				return;
			}

			if (this.fingerCnt == 0)
			{ /* first finger touch input */
				if (addFingerPoint(originX, originY, x, y) == -1) {
					return;
				}

				mouseEventData = new MouseEventData(
						MouseButtonType.LEFT.value(), MouseEventType.PRESS.value(),
						originX, originY, x, y, 0);
				communicator.sendToQEMU(SendCommand.SEND_MOUSE_EVENT, mouseEventData);
			}
			else if ((finger = getFingerPointSearch(x, y)) != null)
			{ /* check the position of previous touch event */
				/* finger point is selected */
				this.grabFingerID = finger.id;
				logger.info(String.format("id %d finger is grabbed\n", this.grabFingerID));
			}
			else if (this.fingerCnt == this.fingerCntMax)
			{ /* Let's assume that this event is last finger touch input */
				finger = getFingerPointFromSlot(this.fingerCntMax - 1);
				if (finger != null) {
	        		mouseEventData = new MouseEventData(
							MouseButtonType.LEFT.value(), MouseEventType.RELEASE.value(),
							originX, originY, finger.x, finger.y, this.fingerCntMax - 1);
					communicator.sendToQEMU(SendCommand.SEND_MOUSE_EVENT, mouseEventData);

					finger.originX = originX;
					finger.originY = originY;
					finger.x = x;
					finger.y = y;

					if (finger.id != 0) {
						mouseEventData = new MouseEventData(
								MouseButtonType.LEFT.value(), MouseEventType.PRESS.value(),
								originX, originY, x, y, this.fingerCntMax - 1);
						communicator.sendToQEMU(SendCommand.SEND_MOUSE_EVENT, mouseEventData);
					}
				}
			}
			else
			{ /* one more finger */
				addFingerPoint(originX, originY, x, y);
				mouseEventData = new MouseEventData(
						MouseButtonType.LEFT.value(), MouseEventType.PRESS.value(),
						originX, originY, x, y, this.fingerCnt - 1);
				communicator.sendToQEMU(SendCommand.SEND_MOUSE_EVENT, mouseEventData);
			}
		} else if (touchType == MouseEventType.RELEASE.value()) { /* released */
			logger.info("mouse up for multi touch");
			this.grabFingerID = 0;
		}
	}

	public void maruFingerProcessing2(
			int touchType, int originX, int originY, int x, int y) {
		FingerPoint finger = null;
		MouseEventData mouseEventData;

		if (touchType == MouseEventType.PRESS.value() ||
				touchType == MouseEventType.DRAG.value()) { /* pressed */
			if (this.grabFingerID > 0) {
				finger = getFingerPointFromSlot(grabFingerID - 1);
				if (finger != null) {
					int originDistanceX = originX - finger.originX;
					int originDistanceY = originY - finger.originY;
					int distanceX = x - finger.x;
					int distanceY = y - finger.y;

					int currrntScreenW = currentState.getCurrentResolutionWidth();
					int currrntScreenH = currentState.getCurrentResolutionHeight();
					int tempFingerX, tempFingerY;

					int i;
					/* bounds checking */                                             
					for (i = 0; i < this.fingerCnt; i++) {
						finger = getFingerPointFromSlot(i);
						if (finger != null) {
							tempFingerX = finger.x + distanceX;
							tempFingerY = finger.y + distanceY;

							if (tempFingerX > currrntScreenW || tempFingerX < 0 ||
									tempFingerY > currrntScreenH || tempFingerY < 0) {
								logger.info(String.format(
										"id %d finger is out of bounds (%d, %d)\n",
										i + 1, tempFingerX, tempFingerY));
								return;
							}
						}
					}

					for (i = 0; i < this.fingerCnt; i++) {
						finger = getFingerPointFromSlot(i);
						if (finger != null) {
							finger.originX += originDistanceX;
							finger.originY += originDistanceY;
							finger.x += distanceX;
							finger.y += distanceY;

							if (finger.id != 0) {
								mouseEventData = new MouseEventData(
										MouseButtonType.LEFT.value(), MouseEventType.PRESS.value(),
										originX, originY, finger.x, finger.y, i);
								communicator.sendToQEMU(SendCommand.SEND_MOUSE_EVENT, mouseEventData);

								/* logger.info(String.format(
										"id %d finger multi-touch dragging = (%d, %d)",
										i + 1, finger.x, finger.y));*/
							}

							try {
								Thread.sleep(2);
							} catch (InterruptedException e) {
								e.printStackTrace();
							}
						}
					}
				}

				return;
			}

			if (this.fingerCnt == 0)
			{ /* first finger touch input */
				if (this.addFingerPoint(originX, originY, x, y) == -1) {
					return;
				}

				mouseEventData = new MouseEventData(
						MouseButtonType.LEFT.value(), MouseEventType.PRESS.value(),
						originX, originY, x, y, 0);
				communicator.sendToQEMU(SendCommand.SEND_MOUSE_EVENT, mouseEventData);
			}
			else if ((finger = this.getFingerPointSearch(x, y)) != null)
			{ /* check the position of previous touch event */
				/* finger point is selected */
				this.grabFingerID = finger.id;
		    	logger.info(String.format("id %d finger is grabbed\n", this.grabFingerID));
			}
			else if (this.fingerCnt == this.fingerCntMax)
			{  /* Let's assume that this event is last finger touch input */
				/* do nothing */
			}
			else /* one more finger */
			{
				addFingerPoint(originX, originY, x, y);
				mouseEventData = new MouseEventData(
						MouseButtonType.LEFT.value(), MouseEventType.PRESS.value(),
						originX, originY, x, y, this.fingerCnt - 1);
				communicator.sendToQEMU(SendCommand.SEND_MOUSE_EVENT, mouseEventData);
			}
		} else if (touchType == MouseEventType.RELEASE.value()) { /* released */
			logger.info("mouse up for multi touch");
			this.grabFingerID = 0;
		}
	}

	private Boolean CalculateOriginCoordinates(
			int ScaledLcdWitdh, int ScaledLcdHeight,
			double scaleFactor, int rotationType, FingerPoint finger) {

		int pointX, pointY, rotatedPointX, rotatedPointY, flag;
		flag = 0;

		/* logger.info("ScaledLcdWitdh:" + ScaledLcdWitdh +
				" ScaledLcdHeight:" + ScaledLcdHeight +
				" scaleFactor:" + scaleFactor + " rotationType:" + rotationType); */

		rotatedPointX = pointX = (int)(finger.x * scaleFactor);
		rotatedPointY = pointY = (int)(finger.y * scaleFactor);

		/* logger.info("rotatedPointX:" + rotatedPointX +
				" rotatedPointY:" + rotatedPointY); */

		if (rotationType == RotationInfo.LANDSCAPE.id()) {
			rotatedPointX = pointY;
			rotatedPointY = ScaledLcdWitdh - pointX;
		} else if (rotationType == RotationInfo.REVERSE_PORTRAIT.id()) {
			rotatedPointX = ScaledLcdWitdh - pointX;
			rotatedPointY = ScaledLcdHeight - pointY;
		} else if (rotationType == RotationInfo.REVERSE_LANDSCAPE.id()) {
			rotatedPointX = ScaledLcdHeight - pointY;
			rotatedPointY = pointX;
		} else {
			/* PORTRAIT: do nothing */
		}

		if (finger.originX != rotatedPointX) {
			logger.info("finger.originX: " +finger.originX);
			finger.originX = rotatedPointX;
			flag = 1;
		}
		if (finger.originY != rotatedPointY) {
			logger.info("finger.originY: " +finger.originY);
			finger.originY = rotatedPointY;
			flag = 1;
		}

		if (flag != 0) {
			return true;
		}

		return false;
	}

	public int rearrangeFingerPoints(
			int lcdWidth, int lcdHeight, double scaleFactor, int rotationType) {
		int i = 0;
		int count = 0;
		FingerPoint finger = null;

		if (this.multiTouchEnable == 0) {
			return 0;
		}

		scaleFactor = scaleFactor / 100;
		lcdWidth *= scaleFactor;
		lcdHeight *= scaleFactor;

		for (i = 0; i < this.fingerCnt; i++) {
			finger = getFingerPointFromSlot(i);
			if (finger != null && finger.id != 0) {
				if (CalculateOriginCoordinates(
						lcdWidth, lcdHeight, scaleFactor, rotationType, finger) == true) {
					count++;
				}
			}
		}

		if (count != 0) {
			this.grabFingerID = 0;
		}

		return count;
	}

	public void clearFingerSlot() {
		int i = 0;
		FingerPoint finger = null;

		for (i = 0; i < this.fingerCnt; i++) {
			finger = getFingerPointFromSlot(i);
			if (finger != null && finger.id != 0) {
				logger.info(String.format(
						"clear %d, %d, %d", finger.x, finger.y, finger.id - 1));

				MouseEventData mouseEventData = new MouseEventData(
						MouseButtonType.LEFT.value(), MouseEventType.RELEASE.value(),
						0, 0, finger.x, finger.y, finger.id - 1);
				communicator.sendToQEMU(SendCommand.SEND_MOUSE_EVENT, mouseEventData);
			}

			finger.id = 0;
			finger.originX = finger.originY = finger.x = finger.y = -1;
		}

		this.grabFingerID = 0;
		this.fingerCnt = 0;
		logger.info("clear multi touch");
	}

	public void cleanup_multiTouchState() {
		this.multiTouchEnable = 0;
		clearFingerSlot();
		fingerSlotimage.dispose();
	}

	public int getMaxTouchPoint() {
		if (this.maxTouchPoint <= 0) {
			setMaxTouchPoint(1);
		}

		return this.maxTouchPoint;
	}

	public void setMaxTouchPoint(int cnt) {
		if (cnt <=0) {
			cnt = 1;
		}

		this.maxTouchPoint = cnt;			
	}
}
