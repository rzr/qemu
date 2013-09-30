/**
 * Skin Utilities
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

package org.tizen.emulator.skin.util;

import java.io.File;
import java.lang.reflect.Array;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.graphics.Region;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.MessageBox;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.comm.ICommunicator.RotationInfo;
import org.tizen.emulator.skin.comm.ICommunicator.Scale;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.config.EmulatorConfig.SkinPropertiesConstants;
import org.tizen.emulator.skin.dbi.EventInfoType;
import org.tizen.emulator.skin.dbi.KeyMapListType;
import org.tizen.emulator.skin.dbi.KeyMapType;
import org.tizen.emulator.skin.dbi.RegionType;
import org.tizen.emulator.skin.dbi.RotationType;
import org.tizen.emulator.skin.image.ProfileSkinImageRegistry;
import org.tizen.emulator.skin.image.ProfileSkinImageRegistry.SkinImageType;
import org.tizen.emulator.skin.layout.HWKey;
import org.tizen.emulator.skin.log.SkinLogger;


/**
 * 
 *
 */
public class SkinUtil {

	public static final String GTK_OS_CLASS = "org.eclipse.swt.internal.gtk.OS";
	public static final String WIN32_OS_CLASS = "org.eclipse.swt.internal.win32.OS";
	public static final String COCOA_OS_CLASS = "org.eclipse.swt.internal.cocoa.OS";

	public static final int UNKNOWN_KEYCODE = -1;
	public static final int SCALE_CONVERTER = 100;
	public static final String EMULATOR_PREFIX = "emulator";

	private static Logger logger =
			SkinLogger.getSkinLogger(SkinUtil.class).getLogger();

	private SkinUtil() {
		/* do nothing */
	}

	public static String getVmName(EmulatorConfig config) {
		String vmPath = config.getArg(ArgsConstants.VM_PATH);

		String regex = "";
		if (SwtUtil.isWindowsPlatform()) {
			regex = "\\" + File.separator;
		} else {
			regex = File.separator;
		}

		String[] split = StringUtil.nvl(vmPath).split(regex);
		String vmName = split[split.length - 1];

		return vmName;
	}

	public static String makeEmulatorName(EmulatorConfig config) {
		String vmName = getVmName(config);

		if (StringUtil.isEmpty(vmName)) {
			vmName = EMULATOR_PREFIX;
		}

		int portNumber = config.getArgInt(ArgsConstants.VM_BASE_PORT) + 1;

		return vmName + ":" + portNumber;
	}

	public static String getSdbPath() {
		String sdbPath = null;

		if (SwtUtil.isWindowsPlatform()) {
			sdbPath = ".\\..\\..\\ansicon.exe";
		} else {
			sdbPath = "./../../sdb";
		}

		return sdbPath;
	}

	public static String getEcpPath() {
		return "emulator-control-panel.jar";
	}

	public static String getSdkVersionFilePath() {
		return ".." + File.separator + "etc" + File.separator + "version";
	}

	public static List<KeyMapType> getHWKeyMapList(short rotationId) {
		RotationType rotation = SkinRotation.getRotation(rotationId);
		if (rotation == null) {
			return null;
		}

		KeyMapListType list = rotation.getKeyMapList();
		if (list == null) {
			/* try to using a KeyMapList of portrait */
			rotation = SkinRotation.getRotation(RotationInfo.PORTRAIT.id());
			if (rotation == null) {
				return null;
			}

			list = rotation.getKeyMapList();
			if (list == null) {
				return null;
			}
		}

		return list.getKeyMap();
	}

	public static HWKey getHWKey(
			int currentX, int currentY, short rotationId, int scale) {
		float convertedScale = convertScale(scale);

		List<KeyMapType> keyMapList = getHWKeyMapList(rotationId);
		if (keyMapList == null) {
			return null;
		}

		for (KeyMapType keyMap : keyMapList) {
			RegionType region = keyMap.getRegion();

			int scaledX = (int) (region.getLeft() * convertedScale);
			int scaledY = (int) (region.getTop() * convertedScale);
			int scaledWidth = (int) (region.getWidth() * convertedScale);
			int scaledHeight = (int) (region.getHeight() * convertedScale);

			if (isInGeometry(currentX, currentY, scaledX, scaledY, scaledWidth, scaledHeight)) {
				EventInfoType eventInfo = keyMap.getEventInfo();

				HWKey hwKey = new HWKey();
				hwKey.setKeyCode(eventInfo.getKeyCode());
				hwKey.setRegion(new SkinRegion(scaledX, scaledY, scaledWidth, scaledHeight));
				hwKey.setTooltip(keyMap.getTooltip());

				return hwKey;
			}
		}

		return null;
	}

	public static boolean isInGeometry(int currentX, int currentY,
			int targetX, int targetY, int targetWidth, int targetHeight) {

		if ((currentX >= targetX) && (currentY >= targetY)) {
			if ((currentX <= (targetX + targetWidth)) &&
					(currentY <= (targetY + targetHeight))) {
				return true;
			}
		}

		return false;
	}

	public static void trimShell(Shell shell, Image image) {
		/* trim transparent pixels in image.
		 * especially, corner round areas. */
		if (null == image) {
			return;
		}

		ImageData imageData = image.getImageData();

		int width = imageData.width;
		int height = imageData.height;

		Region region = new Region();
		region.add(new Rectangle(0, 0, width, height));

		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				int alpha = imageData.getAlpha(i, j);
				if (0 == alpha) {
					region.subtract(i, j, 1, 1);
				}
			}
		}

		shell.setRegion(region);
	}

	public static void trimShell(Shell shell, Image image,
			int left, int top, int width, int height) {
		if (null == image) {
			return;
		}

		ImageData imageData = image.getImageData();

		int right = left + width;
		int bottom = top + height;

		Region region = shell.getRegion();
		if (region == null) {
			return;
		}

		for (int i = left; i < right; i++) {
			for (int j = top; j < bottom; j++) {
				int alpha = imageData.getAlpha(i, j);
				if (0 == alpha) {
					region.subtract(i, j, 1, 1);
				} else {
					region.add(i, j, 1, 1);
				}
			}
		}

		shell.setRegion(region);
	}

	public static int[] convertMouseGeometry( int originalX, int originalY, int lcdWidth, int lcdHeight, int scale,
			int angle ) {

		float convertedScale = convertScale( scale );

		int x = (int) ( originalX * ( 1 / convertedScale ) );
		int y = (int) ( originalY * ( 1 / convertedScale ) );

		int rotatedX = x;
		int rotatedY = y;

		if ( RotationInfo.LANDSCAPE.angle() == angle ) {
			rotatedX = lcdWidth - y;
			rotatedY = x;
		} else if ( RotationInfo.REVERSE_PORTRAIT.angle() == angle ) {
			rotatedX = lcdWidth - x;
			rotatedY = lcdHeight - y;
		} else if ( RotationInfo.REVERSE_LANDSCAPE.angle() == angle ) {
			rotatedX = y;
			rotatedY = lcdHeight - x;
		}

		return new int[] { rotatedX, rotatedY };

	}

	public static Image createScaledImage(Display display,
			ProfileSkinImageRegistry imageRegistry, SkinImageType type,
			short rotationId, int scale) {
		Image imageOrigin = imageRegistry.getSkinImage(rotationId, type);
		if (imageOrigin == null) {
			return null;
		}

		ImageData imageDataSrc = imageOrigin.getImageData();
		ImageData imageDataDst = (ImageData) imageDataSrc.clone();

		float convertedScale = convertScale(scale);

		int width = (int) (imageDataSrc.width * convertedScale);
		int height = (int) (imageDataSrc.height * convertedScale);
		imageDataDst = imageDataDst.scaledTo(width, height);

		return new Image(display, imageDataDst);
	}

	public static float convertScale(int scale) {
		return (float) scale / SCALE_CONVERTER;
	}

	public static int getValidScale( EmulatorConfig config ) {

		int storedScale = config.getSkinPropertyInt( SkinPropertiesConstants.WINDOW_SCALE,
				EmulatorConfig.DEFAULT_WINDOW_SCALE );

		if ( !SkinUtil.isValidScale( storedScale ) ) {
			return EmulatorConfig.DEFAULT_WINDOW_SCALE;
		}else {
			return storedScale;
		}
		
	}

	public static boolean isValidScale( int scale ) {
		if ( Scale.SCALE_100.value() == scale || Scale.SCALE_75.value() == scale
		|| Scale.SCALE_50.value() == scale || Scale.SCALE_25.value() == scale ) {
			return true;
		} else {
			return false;
		}
	}

	public static <T> int openMessage(Shell shell,
			String title, String message, int style, EmulatorConfig config) {
		MessageBox messageBox = new MessageBox(shell, style);

		if (!StringUtil.isEmpty(title)) {
			messageBox.setText(title);
		} else {
			messageBox.setText(makeEmulatorName(config));
		}

		messageBox.setMessage(StringUtil.nvl(message));
		return messageBox.open();
	}

	/* for java reflection */
	private static Field getOSField(String field) {
		String className = "";

		if (SwtUtil.isLinuxPlatform()) {
			className = GTK_OS_CLASS;
		} else if (SwtUtil.isWindowsPlatform()) {
			className = WIN32_OS_CLASS;
		} else if (SwtUtil.isMacPlatform()) {
			className = COCOA_OS_CLASS;
		}

		Field f = null;
		try {
			f = Class.forName(className).getField(field);
		} catch (ClassNotFoundException e) {
			logger.log(Level.SEVERE, e.getMessage(), e);
		} catch (SecurityException e) {
			logger.log(Level.SEVERE, e.getMessage(), e);
		} catch (NoSuchFieldException e) {
			logger.log(Level.SEVERE, e.getMessage(), e);
		}

		return f;
	}

	private static Method getOSMethod(String method, Class<?>... parameterTypes) {
		String className = "";

		if (SwtUtil.isLinuxPlatform()) {
			className = GTK_OS_CLASS;
		} else if (SwtUtil.isWindowsPlatform()) {
			className = WIN32_OS_CLASS;
		} else if (SwtUtil.isMacPlatform()) {
			className = COCOA_OS_CLASS;
		}

		Method m = null;
		try {
			m = Class.forName(className).getMethod(method, parameterTypes);
		} catch (ClassNotFoundException e) {
			logger.log(Level.SEVERE, e.getMessage(), e);
		} catch (SecurityException e) {
			logger.log(Level.SEVERE, e.getMessage(), e);
		} catch (NoSuchMethodException e) {
			logger.log(Level.SEVERE, e.getMessage(), e);
		}

		return m;
	}

	private static Method getOSMethod(String method) {
		return getOSMethod(method, new Class<?>[] {});
	}

	private static Object invokeOSMethod(Method method, Object... args) {
		if (null == method) {
			return null;
		}

		try {
			return method.invoke(null, args);
		} catch (IllegalArgumentException e) {
			logger.log(Level.SEVERE, e.getMessage(), e);
		} catch (IllegalAccessException e) {
			logger.log(Level.SEVERE, e.getMessage(), e);
		} catch (InvocationTargetException e) {
			logger.log(Level.SEVERE, e.getMessage(), e);
		}

		return null;
	}

	private static Object invokeOSMethod(Method method) {
		return invokeOSMethod(method, new Object[] {});
	}

	private static boolean setTopMost32(Shell shell, boolean isOnTop) {

		if (SwtUtil.isLinuxPlatform()) {
			/* reference :
			http://wmctrl.sourcearchive.com/documentation/1.07/main_8c-source.html */

			/* if (!OS.GDK_WINDOWING_X11()) {
				logger.warning("There is no x11 system.");
				return;
			}

			int eventData0 = isOnTop ? 1 : 0; // 'add' or 'remove'
			int topHandle = 0;

			Method m = null;
			try {
				m = Shell.class.getDeclaredMethod("topHandle", new Class<?>[] {});
				m.setAccessible(true);
				topHandle = (Integer) m.invoke( shell, new Object[] {});
			} catch (SecurityException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
			} catch (NoSuchMethodException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
			} catch (IllegalArgumentException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
			} catch (IllegalAccessException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
			} catch (InvocationTargetException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
			}

			int xWindow = OS.gdk_x11_drawable_get_xid(
					OS.GTK_WIDGET_WINDOW(topHandle));
			int xDisplay = OS.GDK_DISPLAY();

			byte[] messageBuffer = Converter.wcsToMbcs(null, "_NET_WM_STATE", true);
			int xMessageAtomType = OS.XInternAtom(xDisplay, messageBuffer, false);

			messageBuffer = Converter.wcsToMbcs(null, "_NET_WM_STATE_ABOVE", true);
			int xMessageAtomAbove = OS.XInternAtom(xDisplay, messageBuffer, false);

			XClientMessageEvent event = new XClientMessageEvent();
			event.type = OS.ClientMessage;
			event.window = xWindow;
			event.message_type = xMessageAtomType;
			event.format = 32;
			event.data[0] = eventData0;
			event.data[1] = xMessageAtomAbove;

			int clientEvent = OS.g_malloc(XClientMessageEvent.sizeof);
			OS.memmove(clientEvent, event, XClientMessageEvent.sizeof);
			int rootWin = OS.XDefaultRootWindow(xDisplay);
			// SubstructureRedirectMask : 1L<<20 | SubstructureNotifyMask : 1L<<19
			OS.XSendEvent(xDisplay, rootWin, false,
					(int) ( 1L << 20 | 1L << 19 ), clientEvent);
			OS.g_free(clientEvent); */


			Boolean gdkWindowingX11 =
					(Boolean) invokeOSMethod(getOSMethod("GDK_WINDOWING_X11"));
			if (null == gdkWindowingX11) {
				logger.warning("GDK_WINDOWING_X11 returned null. There is no x11 system.");
				return false;
			}

			int eventData0 = isOnTop ? 1 : 0; /* 'add' or 'remove' */
			int topHandle = 0;

			Method m = null;
			try {
				m = Shell.class.getDeclaredMethod("topHandle", new Class<?>[] {});

				m.setAccessible(true);
				topHandle = (Integer) m.invoke(shell, new Object[] {});
			} catch (SecurityException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			} catch (NoSuchMethodException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			} catch (IllegalArgumentException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			} catch (IllegalAccessException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			} catch (InvocationTargetException ex ) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			}

			Integer gtkWidgetWindow = (Integer) invokeOSMethod(
					getOSMethod("GTK_WIDGET_WINDOW", int.class), topHandle);
			if (null == gtkWidgetWindow) {
				logger.warning("GTK_WIDGET_WINDOW returned null");
				return false;
			}

			Integer xWindow = (Integer) invokeOSMethod(
					getOSMethod("gdk_x11_drawable_get_xid", int.class), gtkWidgetWindow);
			if (null == xWindow) {
				logger.warning("gdk_x11_drawable_get_xid returned null");
				return false;
			}

			Integer xDisplay = (Integer) invokeOSMethod(getOSMethod("GDK_DISPLAY"));
			if (null == xDisplay) {
				logger.warning("GDK_DISPLAY returned null");
				return false;
			}

			Method xInternAtom = getOSMethod(
					"XInternAtom", int.class, byte[].class, boolean.class);

			Class<?> converterClass = null;
			try {
				converterClass = Class.forName("org.eclipse.swt.internal.Converter");
			} catch (ClassNotFoundException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			}

			Method wcsToMbcs = null;
			byte[] messageBufferState = null;
			byte[] messageBufferAbove = null;

			try {
				wcsToMbcs = converterClass.getMethod(
						"wcsToMbcs", String.class, String.class, boolean.class);
			} catch (SecurityException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			} catch (NoSuchMethodException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			}

			try {
				messageBufferState =
						(byte[]) wcsToMbcs.invoke(null, null, "_NET_WM_STATE", true);
				messageBufferAbove =
						(byte[]) wcsToMbcs.invoke(null, null, "_NET_WM_STATE_ABOVE", true);
			} catch (IllegalArgumentException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			} catch (IllegalAccessException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			} catch (InvocationTargetException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			}

			Integer xMessageAtomType =
					(Integer) invokeOSMethod(xInternAtom, xDisplay, messageBufferState, false);
			if (null == xMessageAtomType) {
				logger.warning("xMessageAtomType is null");
				return false;
			}

			Integer xMessageAtomAbove =
					(Integer) invokeOSMethod(xInternAtom, xDisplay, messageBufferAbove, false);
			if (null == xMessageAtomAbove) {
				logger.warning("xMessageAtomAbove is null");
				return false;
			}

			Class<?> eventClazz = null;
			Object event = null;
			try {
				eventClazz = Class.forName("org.eclipse.swt.internal.gtk.XClientMessageEvent");
				event = eventClazz.newInstance();
			} catch (InstantiationException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
			} catch (IllegalAccessException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
			} catch (ClassNotFoundException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
			}

			if (null == eventClazz || null == event) {
				return false;
			}

			Integer malloc = null;
			try {
				Field type = eventClazz.getField("type");

				Field clientMessageField = getOSField("ClientMessage");
				if (null == clientMessageField) {
					logger.warning("clientMessageField is null");
					return false;
				}
				type.set(event, clientMessageField.get(null));

				Field window = eventClazz.getField("window");
				window.set(event, xWindow);
				Field messageType = eventClazz.getField("message_type");
				messageType.set(event, xMessageAtomType);
				Field format = eventClazz.getField("format");
				format.set(event, 32);

				Object data = Array.newInstance(int.class, 5);
				Array.setInt(data, 0, eventData0);
				Array.setInt(data, 1, xMessageAtomAbove);
				Array.setInt(data, 2, 0);
				Array.setInt(data, 3, 0);
				Array.setInt(data, 4, 0);

				Field dataField = eventClazz.getField("data");
				dataField.set(event, data);

				Field sizeofField = eventClazz.getField("sizeof");
				Integer sizeof = (Integer) sizeofField.get(null);

				Method gMalloc = getOSMethod("g_malloc", int.class);
				malloc = (Integer) invokeOSMethod(gMalloc, sizeof);

				Method memmove =
						getOSMethod("memmove", int.class, eventClazz, int.class);
				invokeOSMethod(memmove, malloc, event, sizeof);

			} catch (NoSuchFieldException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			} catch (IllegalAccessException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			}

			Method xDefaultRootWindow =
					getOSMethod("XDefaultRootWindow", int.class);
			Integer rootWin =
					(Integer) invokeOSMethod(xDefaultRootWindow, xDisplay);

			Method xSendEvent = getOSMethod("XSendEvent",
					int.class, int.class, boolean.class, int.class, int.class);

			/* SubstructureRedirectMask : 1L<<20 | SubstructureNotifyMask : 1L<<19 */
			invokeOSMethod(xSendEvent, xDisplay, rootWin,
					false, (int) (1L << 20 | 1L << 19), malloc);
			invokeOSMethod(getOSMethod("g_free", int.class), malloc);

		} else if (SwtUtil.isWindowsPlatform()) {
			Point location = shell.getLocation();

			/* int hWndInsertAfter = 0;
			if (isOnTop) {
				hWndInsertAfter = OS.HWND_TOPMOST;
			} else {
				hWndInsertAfter = OS.HWND_NOTOPMOST;
			}
			OS.SetWindowPos(shell.handle,
					hWndInsertAfter, location.x, location.y, 0, 0, OS.SWP_NOSIZE); */

			int hWndInsertAfter = 0;
			int noSize = 0;

			try {
				if (isOnTop) {
					Field topMost = getOSField("HWND_TOPMOST");
					if (null == topMost) {
						logger.warning("topMost is null");
						return false;
					}

					hWndInsertAfter = topMost.getInt(null);
				} else {
					Field noTopMost = getOSField("HWND_NOTOPMOST");
					if (null == noTopMost) {
						logger.warning("HWND_NOTOPMOST is null");
						return false;
					}

					hWndInsertAfter = noTopMost.getInt(null);
				}

				Field noSizeField = getOSField("SWP_NOSIZE");
				if (null == noSizeField) {
					logger.warning("SWP_NOSIZE is null");
					return false;
				}

				noSize = noSizeField.getInt(null);

			} catch (IllegalArgumentException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			} catch (IllegalAccessException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			}

			Method m = getOSMethod("SetWindowPos",
					int.class, int.class, int.class,
					int.class, int.class, int.class, int.class);

			/* org.eclipse.swt.widgets.Shell */
			int shellHandle = 0;
			try {
				Field field = shell.getClass().getField("handle");
				shellHandle = field.getInt(shell);
				logger.info("shell.handle:" + shellHandle);
			} catch (IllegalArgumentException e) {
				logger.log(Level.SEVERE, e.getMessage(), e);
				return false;
			} catch (IllegalAccessException e) {
				logger.log(Level.SEVERE, e.getMessage(), e);
				return false;
			} catch (SecurityException e) {
				logger.log(Level.SEVERE, e.getMessage(), e);
				return false;
			} catch (NoSuchFieldException e) {
				logger.log(Level.SEVERE, e.getMessage(), e);
				return false;
			}

			invokeOSMethod(m, shellHandle, hWndInsertAfter,
					location.x, location.y, 0, 0, noSize);
		} else if (SwtUtil.isMacPlatform()) {
			/* do nothing */
			logger.warning("not supported yet");
			return false;
		}

		return true;
	}

	private static boolean setTopMost64(Shell shell, boolean isOnTop) {

		if (SwtUtil.isLinuxPlatform()) {
			Boolean gdkWindowingX11 =
					(Boolean) invokeOSMethod(getOSMethod("GDK_WINDOWING_X11"));
			if (null == gdkWindowingX11) {
				logger.warning("GDK_WINDOWING_X11 returned null. There is no x11 system.");
				return false;
			}

			int eventData0 = isOnTop ? 1 : 0; /* 'add' or 'remove' */
			long topHandle = 0;

			Method m = null;
			try {
				m = Shell.class.getDeclaredMethod("topHandle", new Class<?>[] {});

				m.setAccessible(true);
				topHandle = (Long) m.invoke(shell, new Object[] {});
			} catch (SecurityException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			} catch (NoSuchMethodException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			} catch (IllegalArgumentException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			} catch (IllegalAccessException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			} catch (InvocationTargetException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			}

			Long gtkWidgetWindow = (Long) invokeOSMethod(
					getOSMethod("GTK_WIDGET_WINDOW", long.class), topHandle);
			if (null == gtkWidgetWindow) {
				logger.warning("GTK_WIDGET_WINDOW returned null");
				return false;
			}

			Long xWindow = (Long) invokeOSMethod(
					getOSMethod("gdk_x11_drawable_get_xid", long.class), gtkWidgetWindow);
			if (null == xWindow) {
				logger.warning("gdk_x11_drawable_get_xid returned null");
				return false;
			}

			Long xDisplay = (Long) invokeOSMethod( getOSMethod("GDK_DISPLAY"));
			if (null == xDisplay) {
				logger.warning("GDK_DISPLAY returned null");
				return false;
			}

			Method xInternAtom = getOSMethod(
					"XInternAtom", long.class, byte[].class, boolean.class);

			Class<?> converterClass = null;
			try {
				converterClass = Class.forName("org.eclipse.swt.internal.Converter");
			} catch (ClassNotFoundException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			}

			Method wcsToMbcs = null;
			byte[] messageBufferState = null;
			byte[] messageBufferAbove = null;

			try {
				wcsToMbcs = converterClass.getMethod(
						"wcsToMbcs", String.class, String.class, boolean.class);
			} catch (SecurityException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			} catch (NoSuchMethodException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			}

			try {
				messageBufferState =
						(byte[]) wcsToMbcs.invoke(null, null, "_NET_WM_STATE", true);
				messageBufferAbove =
						(byte[]) wcsToMbcs.invoke(null, null, "_NET_WM_STATE_ABOVE", true);
			} catch (IllegalArgumentException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			} catch (IllegalAccessException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			} catch (InvocationTargetException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			}

			Long xMessageAtomType =
					(Long) invokeOSMethod(xInternAtom, xDisplay, messageBufferState, false);
			if (null == xMessageAtomType) {
				logger.warning("xMessageAtomType is null");
				return false;
			}

			Long xMessageAtomAbove =
					(Long) invokeOSMethod(xInternAtom, xDisplay, messageBufferAbove, false);
			if (null == xMessageAtomAbove) {
				logger.warning("xMessageAtomAbove is null");
				return false;
			}

			Class<?> eventClazz = null;
			Object event = null;
			try {
				eventClazz = Class.forName("org.eclipse.swt.internal.gtk.XClientMessageEvent");
				event = eventClazz.newInstance();
			} catch (InstantiationException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
			} catch (IllegalAccessException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
			} catch (ClassNotFoundException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
			}

			if (null == eventClazz || null == event) {
				return false;
			}

			Long malloc = null;
			try {
				Field type = eventClazz.getField("type");

				Field clientMessageField = getOSField("ClientMessage");
				if (null == clientMessageField) {
					logger.warning("clientMessageField is null");
					return false;
				}
				type.set(event, clientMessageField.get(null));

				Field window = eventClazz.getField("window");
				window.set(event, xWindow);
				Field messageType = eventClazz.getField("message_type");
				messageType.set(event, xMessageAtomType);
				Field format = eventClazz.getField("format");
				format.set(event, 32);

				Object data = Array.newInstance(long.class, 5);
				Array.setLong(data, 0, eventData0);
				Array.setLong(data, 1, xMessageAtomAbove);
				Array.setLong(data, 2, 0);
				Array.setLong(data, 3, 0);
				Array.setLong(data, 4, 0);

				Field dataField = eventClazz.getField("data");
				dataField.set(event, data);

				Field sizeofField = eventClazz.getField("sizeof");
				Integer sizeof = (Integer) sizeofField.get(null);

				Method gMalloc = getOSMethod("g_malloc", long.class);
				malloc = (Long) invokeOSMethod(gMalloc, sizeof.longValue());

				Method memmove =
						getOSMethod("memmove", long.class, eventClazz, long.class);
				invokeOSMethod(memmove, malloc, event, sizeof.longValue());

			} catch (NoSuchFieldException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			} catch (IllegalAccessException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			}

			Method xDefaultRootWindow =
					getOSMethod("XDefaultRootWindow", long.class);
			Long rootWin =
					(Long) invokeOSMethod(xDefaultRootWindow, xDisplay);

			Method xSendEvent = getOSMethod("XSendEvent", long.class, long.class, boolean.class,
					long.class, long.class);

			/*  ubstructureRedirectMask : 1L<<20 | SubstructureNotifyMask : 1L<<19 */
			invokeOSMethod(xSendEvent, xDisplay, rootWin,
					false, (long) (1L << 20 | 1L << 19), malloc);
			invokeOSMethod(getOSMethod("g_free", long.class), malloc);
		} else if (SwtUtil.isWindowsPlatform()) {
			Point location = shell.getLocation();

			long hWndInsertAfter = 0;
			int noSize = 0;

			try {
				if (isOnTop) {
					Field topMost = getOSField("HWND_TOPMOST");
					if (null == topMost) {
						logger.warning("topMost is null");
						return false;
					}

					hWndInsertAfter = topMost.getLong(null);
				} else {
					Field noTopMost = getOSField("HWND_NOTOPMOST");
					if (null == noTopMost) {
						logger.warning("noTopMost is null");
						return false;
					}

					hWndInsertAfter = noTopMost.getLong(null);
				}

				Field noSizeField = getOSField("SWP_NOSIZE");
				if (null == noSizeField) {
					logger.warning("SWP_NOSIZE is null");
					return false;
				}

				noSize = noSizeField.getInt(null);

			} catch (IllegalArgumentException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			} catch (IllegalAccessException ex) {
				logger.log(Level.SEVERE, ex.getMessage(), ex);
				return false;
			}

			Method m = getOSMethod("SetWindowPos",
					long.class, long.class, int.class,
					int.class, int.class, int.class, int.class);

			/* org.eclipse.swt.widgets.Shell */
			long shellHandle = 0;
			try {
				Field field = shell.getClass().getField("handle");
				shellHandle = field.getLong(shell);
				logger.info("shell.handle:" + shellHandle);
			} catch (IllegalArgumentException e) {
				logger.log(Level.SEVERE, e.getMessage(), e);
				return false;
			} catch (IllegalAccessException e) {
				logger.log(Level.SEVERE, e.getMessage(), e);
				return false;
			} catch (SecurityException e) {
				logger.log(Level.SEVERE, e.getMessage(), e);
				return false;
			} catch (NoSuchFieldException e) {
				logger.log(Level.SEVERE, e.getMessage(), e);
				return false;
			}

			invokeOSMethod(m, shellHandle, hWndInsertAfter,
					location.x, location.y, 0, 0, noSize);
		} else if (SwtUtil.isMacPlatform()) {
			/* do nothing */
			logger.warning("not supported yet");
			return false;
		}

		return true;
	}

	public static boolean setTopMost(Shell shell, boolean isOnTop) {
		if (shell == null) {
			return false;
		}

		/* internal/Library.java::arch() */
		String osArch = System.getProperty("os.arch"); /* $NON-NLS-1$ */

		if (osArch.equals("amd64") || osArch.equals("x86_64") ||
				osArch.equals("IA64W") || osArch.equals("ia64")) {
			/* $NON-NLS-1$ $NON-NLS-2$ $NON-NLS-3$ */
			logger.info("64bit architecture : " + osArch);

			return setTopMost64(shell, isOnTop); /* 64bit */
		}

		logger.info("32bit architecture : " + osArch);
		return setTopMost32(shell, isOnTop); /* 32bit */
	}
}
