/**
 * Multi-touch
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
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
	private static final int FINGER_POINT_SIZE = 32;
	private static final int FINGER_POINT_ALPHA = 0x7E;

	private static final int RED_MASK = 0x0000FF00;
	private static final int GREEN_MASK = 0x00FF0000;
	private static final int BLUE_MASK = 0xFF000000;
	private static final int COLOR_DEPTH = 32;

	private Logger logger =
			SkinLogger.getSkinLogger(EmulatorFingers.class).getLogger();

	private int multiTouchEnable;
	private int maxTouchPoint;
	protected int fingerCnt;

	private int grabFingerID = 0;
	private ArrayList<FingerPoint> FingerPointList;

	protected int fingerPointSize;
	protected int fingerPointSizeHalf;
	protected Image fingerPointImage;

	protected SocketCommunicator communicator;
	private EmulatorSkinState currentState;

	/**
	 *  Constructor
	 */
	EmulatorFingers(int maximum,
			EmulatorSkinState currentState, SocketCommunicator communicator) {
		this.currentState = currentState;
		this.communicator = communicator;

		initMultiTouchState(maximum);
	}

	class FingerPoint {
		private int id;
		private int originX;
		private int originY;
		private int x;
		private int y;

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
		if (index < 0 || index > getMaxTouchPoint()) {
			return null;
		}

		return FingerPointList.get(index);
	}

	public FingerPoint getFingerPointSearch(int x, int y) {
		FingerPoint fingerPoint = null;
		int fingerRegion = fingerPointSize + 2;
		//logger.info("x: "+x+ "y: "+ y + " fingerRegion: " + fingerRegion);

		for (int i = fingerCnt - 1; i >= 0; i--) {
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

	private void initMultiTouchState(int maximum) {
		multiTouchEnable = 0;

		int fingerCntMax = maximum;
		if (fingerCntMax > MAX_FINGER_CNT) {
			fingerCntMax = MAX_FINGER_CNT;
		}
		setMaxTouchPoint(fingerCntMax);

		logger.info("maxTouchPoint : " + getMaxTouchPoint());
		fingerCnt = 0;

		FingerPointList = new ArrayList<FingerPoint>();
		for (int i = 0; i <= getMaxTouchPoint(); i++) {
			FingerPointList.add(new FingerPoint(-1, -1, -1, -1));
		} 

		this.fingerPointSize = FINGER_POINT_SIZE;
		this.fingerPointSizeHalf = fingerPointSize / 2;

		Color pointOutlineColor = new Color(Display.getCurrent(), 0xDD, 0xDD, 0xDD);
		Color pointColor = new Color(Display.getCurrent(), 0x0F, 0x0F, 0x0F);
		PaletteData palette = new PaletteData(RED_MASK, GREEN_MASK, BLUE_MASK);

		ImageData imageData = new ImageData(
				fingerPointSize + 4, fingerPointSize + 4, COLOR_DEPTH, palette);
		imageData.transparentPixel = 0;
		this.fingerPointImage = new Image(Display.getCurrent(), imageData);

		/* draw point image */
		GC gc = new GC(fingerPointImage);

		gc.setBackground(pointColor);
		gc.fillOval(2, 2, fingerPointSize, fingerPointSize);
		gc.setForeground(pointOutlineColor);
		gc.drawOval(0, 0, fingerPointSize + 2, fingerPointSize + 2);

		gc.dispose();
		pointOutlineColor.dispose();
		pointColor.dispose();
	}

	public void setMultiTouchEnable(int value) {
		multiTouchEnable = value;
	}

	public int getMultiTouchEnable() {
		return multiTouchEnable;
	}

	protected int addFingerPoint(int originX, int originY, int x, int y) {
		if (fingerCnt == getMaxTouchPoint()) {
			logger.warning("support multi-touch up to "
					+ getMaxTouchPoint() + " fingers");
			return -1;
		}

		fingerCnt += 1;

		FingerPointList.get(fingerCnt - 1).id = fingerCnt;
		FingerPointList.get(fingerCnt - 1).originX = originX;
		FingerPointList.get(fingerCnt - 1).originY = originY;
		FingerPointList.get(fingerCnt - 1).x = x;
		FingerPointList.get(fingerCnt - 1).y = y;

		logger.info(fingerCnt + " finger touching");

		return fingerCnt;
	}

	protected void drawFingerPoints(GC gc) {
		int alpha = gc.getAlpha();
		gc.setAlpha(FINGER_POINT_ALPHA);

		FingerPoint fingerSlot = null;
		for (int i = 0; i < fingerCnt; i++) {
			fingerSlot = getFingerPointFromSlot(i);

			if (fingerSlot != null) {
				gc.drawImage(fingerPointImage,
						fingerSlot.originX - fingerPointSizeHalf - 2,
						fingerSlot.originY - fingerPointSizeHalf - 2);
			}
		}

		gc.setAlpha(alpha);
	}

	public void maruFingerProcessing1(
			int touchType, int originX, int originY, int x, int y) {
		FingerPoint finger = null;
		MouseEventData mouseEventData = null;

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
						logger.info("id " + grabFingerID + " finger multi-touch dragging : ("
								+ x + ", " + y + ")");

						mouseEventData = new MouseEventData(
								MouseButtonType.LEFT.value(), MouseEventType.PRESS.value(),
								originX, originY, x, y, grabFingerID - 1);
						communicator.sendToQEMU(
								SendCommand.SEND_MOUSE_EVENT, mouseEventData, false);
					} 
				}

				return;
			}

			if (fingerCnt == 0)
			{ /* first finger touch input */
				if (addFingerPoint(originX, originY, x, y) == -1) {
					return;
				}

				mouseEventData = new MouseEventData(
						MouseButtonType.LEFT.value(), MouseEventType.PRESS.value(),
						originX, originY, x, y, 0);
				communicator.sendToQEMU(
						SendCommand.SEND_MOUSE_EVENT, mouseEventData, false);
			}
			else if ((finger = getFingerPointSearch(x, y)) != null)
			{ /* check the position of previous touch event */
				/* finger point is selected */
				grabFingerID = finger.id;
				logger.info("id " + grabFingerID + " finger is grabbed");
			}
			else if (fingerCnt == getMaxTouchPoint())
			{ /* Let's assume that this event is last finger touch input */
				finger = getFingerPointFromSlot(getMaxTouchPoint() - 1);
				if (finger != null) {
	        		mouseEventData = new MouseEventData(
							MouseButtonType.LEFT.value(), MouseEventType.RELEASE.value(),
							originX, originY, finger.x, finger.y, getMaxTouchPoint() - 1);
					communicator.sendToQEMU(
							SendCommand.SEND_MOUSE_EVENT, mouseEventData, false);

					finger.originX = originX;
					finger.originY = originY;
					finger.x = x;
					finger.y = y;

					if (finger.id != 0) {
						mouseEventData = new MouseEventData(
								MouseButtonType.LEFT.value(), MouseEventType.PRESS.value(),
								originX, originY, x, y, getMaxTouchPoint() - 1);
						communicator.sendToQEMU(
								SendCommand.SEND_MOUSE_EVENT, mouseEventData, false);
					}
				}
			}
			else
			{ /* one more finger */
				addFingerPoint(originX, originY, x, y);
				mouseEventData = new MouseEventData(
						MouseButtonType.LEFT.value(), MouseEventType.PRESS.value(),
						originX, originY, x, y, fingerCnt - 1);
				communicator.sendToQEMU(
						SendCommand.SEND_MOUSE_EVENT, mouseEventData, false);
			}
		} else if (touchType == MouseEventType.RELEASE.value()) { /* released */
			logger.info("mouse up for multi touch");
			grabFingerID = 0;
		}
	}

	public void maruFingerProcessing2(
			int touchType, int originX, int originY, int x, int y) {
		FingerPoint finger = null;
		MouseEventData mouseEventData = null;

		if (touchType == MouseEventType.PRESS.value() ||
				touchType == MouseEventType.DRAG.value()) { /* pressed */
			if (grabFingerID > 0) {
				finger = getFingerPointFromSlot(grabFingerID - 1);
				if (finger != null) {
					int originDistanceX = originX - finger.originX;
					int originDistanceY = originY - finger.originY;
					int distanceX = x - finger.x;
					int distanceY = y - finger.y;

					int currrntScreenW = currentState.getCurrentResolutionWidth();
					int currrntScreenH = currentState.getCurrentResolutionHeight();
					int tempFingerX = 0, tempFingerY = 0;

					int i = 0;

					/* bounds checking */                                             
					for (i = 0; i < fingerCnt; i++) {
						finger = getFingerPointFromSlot(i);
						if (finger != null) {
							tempFingerX = finger.x + distanceX;
							tempFingerY = finger.y + distanceY;

							if (tempFingerX > currrntScreenW || tempFingerX < 0 ||
									tempFingerY > currrntScreenH || tempFingerY < 0) {
								logger.info("id " + (i + 1) + " finger is out of bounds : ("
									+ tempFingerX + ", " + tempFingerY + ")");

								return;
							}
						}
					}

					for (i = 0; i < fingerCnt; i++) {
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
								communicator.sendToQEMU(
										SendCommand.SEND_MOUSE_EVENT, mouseEventData, false);

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

			if (fingerCnt == 0)
			{ /* first finger touch input */
				if (addFingerPoint(originX, originY, x, y) == -1) {
					return;
				}

				mouseEventData = new MouseEventData(
						MouseButtonType.LEFT.value(), MouseEventType.PRESS.value(),
						originX, originY, x, y, 0);
				communicator.sendToQEMU(
						SendCommand.SEND_MOUSE_EVENT, mouseEventData, false);
			}
			else if ((finger = getFingerPointSearch(x, y)) != null)
			{ /* check the position of previous touch event */
				/* finger point is selected */
				grabFingerID = finger.id;
				logger.info("id " + grabFingerID + " finger is grabbed");
			}
			else if (fingerCnt == getMaxTouchPoint())
			{  /* Let's assume that this event is last finger touch input */
				/* do nothing */
			}
			else /* one more finger */
			{
				addFingerPoint(originX, originY, x, y);
				mouseEventData = new MouseEventData(
						MouseButtonType.LEFT.value(), MouseEventType.PRESS.value(),
						originX, originY, x, y, fingerCnt - 1);
				communicator.sendToQEMU(
						SendCommand.SEND_MOUSE_EVENT, mouseEventData, false);
			}
		} else if (touchType == MouseEventType.RELEASE.value()) { /* released */
			logger.info("mouse up for multi touch");
			grabFingerID = 0;
		}
	}

	private Boolean calculateOriginCoordinates(
			int scaledDisplayWitdh, int scaledDisplayHeight,
			double scaleFactor, int rotationType, FingerPoint finger) {
		int pointX = 0, pointY = 0;
		int rotatedPointX = 0, rotatedPointY = 0;
		int flag = 0;

		/* logger.info("ScaledLcdWitdh:" + ScaledLcdWitdh +
				" ScaledLcdHeight:" + ScaledLcdHeight +
				" scaleFactor:" + scaleFactor + " rotationType:" + rotationType); */

		rotatedPointX = pointX = (int)(finger.x * scaleFactor);
		rotatedPointY = pointY = (int)(finger.y * scaleFactor);

		/* logger.info("rotatedPointX:" + rotatedPointX +
				" rotatedPointY:" + rotatedPointY); */

		if (rotationType == RotationInfo.LANDSCAPE.id()) {
			rotatedPointX = pointY;
			rotatedPointY = scaledDisplayWitdh - pointX;
		} else if (rotationType == RotationInfo.REVERSE_PORTRAIT.id()) {
			rotatedPointX = scaledDisplayWitdh - pointX;
			rotatedPointY = scaledDisplayHeight - pointY;
		} else if (rotationType == RotationInfo.REVERSE_LANDSCAPE.id()) {
			rotatedPointX = scaledDisplayHeight - pointY;
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
			int displayWidth, int displayHeight, double scaleFactor, int rotationType) {
		if (multiTouchEnable == 0) {
			return 0;
		}

		int count = 0;
		FingerPoint finger = null;

		scaleFactor = scaleFactor / 100;
		displayWidth *= scaleFactor;
		displayHeight *= scaleFactor;

		for (int i = 0; i < fingerCnt; i++) {
			finger = getFingerPointFromSlot(i);
			if (finger != null && finger.id != 0) {
				if (calculateOriginCoordinates(
						displayWidth, displayHeight, scaleFactor, rotationType, finger) == true) {
					count++;
				}
			}
		}

		if (count != 0) {
			grabFingerID = 0;
		}

		return count;
	}

	public void clearFingerSlot() {
		int i = 0;
		FingerPoint finger = null;

		logger.info("clear multi-touch slot");

		for (i = 0; i < fingerCnt; i++) {
			finger = getFingerPointFromSlot(i);
			if (finger != null) {
				if (finger.id > 0) {
					logger.info(String.format(
							"clear %d, %d, %d", finger.x, finger.y, finger.id - 1));

					MouseEventData mouseEventData = new MouseEventData(
							MouseButtonType.LEFT.value(), MouseEventType.RELEASE.value(),
							0, 0, finger.x, finger.y, finger.id - 1);
					communicator.sendToQEMU(
							SendCommand.SEND_MOUSE_EVENT, mouseEventData, false);
				}

				finger.id = 0;
				finger.originX = finger.originY = finger.x = finger.y = -1;
			}
		}

		grabFingerID = 0;
		fingerCnt = 0;
	}

	public void cleanupMultiTouchState() {
		multiTouchEnable = 0;
		clearFingerSlot();

		fingerPointImage.dispose();
	}

	public int getMaxTouchPoint() {
		if (maxTouchPoint <= 0) {
			setMaxTouchPoint(1);
		}

		return maxTouchPoint;
	}

	public void setMaxTouchPoint(int cnt) {
		if (cnt <= 0) {
			cnt = 1;
		}

		maxTouchPoint = cnt;
	}
}
