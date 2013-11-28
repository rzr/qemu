/**
 * Emulator Configuration Information
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

package org.tizen.emulator.skin.config;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.util.Map;
import java.util.Properties;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Display;
import org.tizen.emulator.skin.comm.ICommunicator.RotationInfo;
import org.tizen.emulator.skin.dbi.EmulatorUI;
import org.tizen.emulator.skin.exception.ConfigException;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.log.SkinLogger.SkinLogLevel;
import org.tizen.emulator.skin.util.IOUtil;
import org.tizen.emulator.skin.util.SkinUtil;
import org.tizen.emulator.skin.util.StringUtil;


/**
 * 
 *
 */
public class EmulatorConfig {
	public static final String INVALID_OPTION_MESSAGE =
			"An invalid option have caused the error.\n";

	public static final int DEFAULT_WINDOW_SCALE = 50;
	public static final int MIN_SCALE_FACTOR = 25;
	public static final int MAX_SCALE_FACTOR = 200;
	public static final short DEFAULT_WINDOW_ROTATION = RotationInfo.PORTRAIT.id();
	public static final int DEFAULT_WINDOW_X = 50;
	public static final int DEFAULT_WINDOW_Y = 50;
	public static final SkinLogLevel DEFAULT_LOG_LEVEL = SkinLogLevel.DEBUG;

	private static Logger logger =
			SkinLogger.getSkinLogger(EmulatorConfig.class).getLogger();

	public interface ArgsConstants {
		public static final String SIMPLE_MESSAGE = "simple.msg";
		public static final String UID = "uid";
		public static final String RESOLUTION_WIDTH = "width";
		public static final String RESOLUTION_HEIGHT = "height";
		public static final String HEART_BEAT_IGNORE = "hb.ignore";
		public static final String LOG_LEVEL = "log.level";
		public static final String VM_PATH = "vm.path";
		public static final String VM_SKIN_PORT = "vm.skinport";
		public static final String VM_BASE_PORT = "vm.baseport";
		public static final String SKIN_PATH = "skin.path";
		public static final String DISPLAY_SHM = "display.shm";
		public static final String INPUT_MOUSE = "input.mouse";
		public static final String INPUT_TOUCHSCREEN = "input.touch";
		public static final String INPUT_TOUCH_MAXPOINT = "input.touch.maxpoint";
	}

	public interface SkinPropertiesConstants {
		public static final String WINDOW_X = "window.x";
		public static final String WINDOW_Y = "window.y";
		public static final String WINDOW_ROTATION = "window.rotate";
		public static final String WINDOW_SCALE = "window.scale";
		public static final String WINDOW_ONTOP = "window.ontop"; /* always on top */
		public static final String WINDOW_INTERPOLATION = "window.interpolation";
		public static final String KEYWINDOW_POSITION = "window.keywindow.position";
	}

	public interface ConfigPropertiesConstants {
		public static final String HEART_BEAT_IGNORE = "hb.ignore";
		public static final String LOG_LEVEL = "log.level";
	}

	public interface SkinInfoConstants {
		public static final String SDK_VERSION_NAME = "sdk.version-name"; /* from version file */
		public static final String SKIN_NAME = "skin.name";
		public static final String RESOLUTION_WIDTH = "resolution.width";
		public static final String RESOLUTION_HEIGHT = "resolution.height";
		public static final String MANAGER_PRIORITY = "manager.priority";
	}

	private Map<String, String> args;
	private EmulatorUI dbiContents;
	private Properties skinProperties;
	private Properties configProperties;
	private String skinPropertiesFilePath;

	/**
	 *  Constructor
	 */
	public EmulatorConfig(Map<String, String> args,
			EmulatorUI dbiContents, Properties skinProperties,
			String skinPropertiesFilePath, Properties configProperties) {
		this.args = args;
		this.dbiContents = dbiContents;
		this.skinProperties = skinProperties;
		this.skinPropertiesFilePath = skinPropertiesFilePath;
		this.configProperties = configProperties;

		if (null == configProperties) {
			this.configProperties = new Properties();
		}

		/* load SDK version */
		String strVersion = "Undefined";
		String versionFilePath = SkinUtil.getSdkVersionFilePath();

		File file = new File(versionFilePath);

		if (file.exists() == false || file.isFile() == false) {
			logger.warning("cannot read version from " + versionFilePath);
		} else {
			BufferedReader reader = null;
			try {
				reader = new BufferedReader(new FileReader(file));
			} catch (FileNotFoundException e) {
				logger.warning(e.getMessage());
			}

			if (reader != null) {
				try {
					strVersion = reader.readLine();
				} catch (IOException e) {
					logger.warning(e.getMessage());
				}

				try {
					reader.close();
				} catch (IOException e) {
					logger.warning(e.getMessage());
				}
			}
		}

		logger.info("SDK version : " + strVersion);
		setSkinProperty(
				EmulatorConfig.SkinInfoConstants.SDK_VERSION_NAME, strVersion);
	}

	public static void validateArgs(Map<String, String> args) throws ConfigException {
		if (null == args) {
			String msg = INVALID_OPTION_MESSAGE;
			throw new ConfigException(msg);
		}

		if (args.containsKey(ArgsConstants.UID)) {
			String uid = args.get(ArgsConstants.UID);
			try {
				Integer.parseInt(uid);
			} catch (NumberFormatException e) {
				String msg = INVALID_OPTION_MESSAGE + "The \'" +
						ArgsConstants.UID +
						"\' argument is not numeric.\n: " + uid;
				throw new ConfigException(msg);
			}
		}

		if (args.containsKey(ArgsConstants.VM_SKIN_PORT)) {
			String serverPort = args.get(ArgsConstants.VM_SKIN_PORT);
			try {
				Integer.parseInt(serverPort);
			} catch (NumberFormatException e) {
				String msg = INVALID_OPTION_MESSAGE + "The \'" +
						ArgsConstants.VM_SKIN_PORT +
						"\' argument is not numeric.\n: " + serverPort;
				throw new ConfigException(msg);
			}
		} else {
			String msg = INVALID_OPTION_MESSAGE + "The \'" +
					ArgsConstants.VM_SKIN_PORT + "\' is required argument.";
			throw new ConfigException(msg);
		}

		if (args.containsKey(ArgsConstants.RESOLUTION_WIDTH)) {
			String width = args.get(ArgsConstants.RESOLUTION_WIDTH);
			try {
				Integer.parseInt(width);
			} catch (NumberFormatException e) {
				String msg = INVALID_OPTION_MESSAGE + "The \'" +
						ArgsConstants.RESOLUTION_WIDTH +
						"\' argument is not numeric.\n: " + width;
				throw new ConfigException(msg);
			}
		} else {
			String msg = INVALID_OPTION_MESSAGE + "The \'" +
					ArgsConstants.RESOLUTION_WIDTH + "\' is required argument.";
			throw new ConfigException(msg);
		}

		if (args.containsKey(ArgsConstants.RESOLUTION_HEIGHT)) {
			String height = args.get(ArgsConstants.RESOLUTION_HEIGHT);
			try {
				Integer.parseInt(height);
			} catch (NumberFormatException e) {
				String msg = INVALID_OPTION_MESSAGE + "The \'" +
						ArgsConstants.RESOLUTION_HEIGHT +
						"\' argument is not numeric.\n: " + height;
				throw new ConfigException(msg);
			}
		} else {
			String msg = INVALID_OPTION_MESSAGE + "The \'" +
					ArgsConstants.RESOLUTION_HEIGHT + "\' is required argument.";
			throw new ConfigException(msg);
		}
	}

	public static void validateSkinProperties(
			Properties skinProperties) throws ConfigException {
		if (null == skinProperties || 0 == skinProperties.size()) {
			return;
		}

		if (skinProperties.containsKey(
				SkinPropertiesConstants.WINDOW_X) == true) {
			String window_x = skinProperties.getProperty(SkinPropertiesConstants.WINDOW_X);

			try {
				Integer.parseInt(window_x);
			} catch (NumberFormatException e) {
				String msg = SkinPropertiesConstants.WINDOW_X +
						" in .skin.properties is not numeric : " + window_x;
				logger.warning(msg);

				skinProperties.remove(SkinPropertiesConstants.WINDOW_X);
			}
		}

		if (skinProperties.containsKey(
				SkinPropertiesConstants.WINDOW_Y) == true) {
			String window_y = skinProperties.getProperty(SkinPropertiesConstants.WINDOW_Y);

			try {
				Integer.parseInt(window_y);
			} catch (NumberFormatException e) {
				String msg = SkinPropertiesConstants.WINDOW_Y +
						" in .skin.properties is not numeric. : " + window_y;
				logger.warning(msg);

				skinProperties.remove(SkinPropertiesConstants.WINDOW_Y);
			}
		}

		if (skinProperties.containsKey(
				SkinPropertiesConstants.WINDOW_ROTATION) == true) {
			String rotation = skinProperties.getProperty(SkinPropertiesConstants.WINDOW_ROTATION);

			try {
				Integer.parseInt(rotation);
			} catch (NumberFormatException e) {
				String msg = SkinPropertiesConstants.WINDOW_ROTATION +
						" in .skin.properties is not numeric. : " + rotation;
				logger.warning(msg);

				skinProperties.remove(SkinPropertiesConstants.WINDOW_ROTATION);
			}
		}

		if (skinProperties.containsKey(
				SkinPropertiesConstants.WINDOW_SCALE) == true) {
			String scale = skinProperties.getProperty(SkinPropertiesConstants.WINDOW_SCALE);

			try {
				Integer.parseInt(scale);
			} catch (NumberFormatException e) {
				String msg = SkinPropertiesConstants.WINDOW_SCALE +
						" in .skin.properties is not numeric. : " + scale;
				logger.warning(msg);

				skinProperties.remove(SkinPropertiesConstants.WINDOW_SCALE);
			}
		}

		if (skinProperties.containsKey(
				SkinPropertiesConstants.KEYWINDOW_POSITION) == true) {
			String position = skinProperties.getProperty(SkinPropertiesConstants.KEYWINDOW_POSITION);

			try {
				Integer.parseInt(position);
			} catch (NumberFormatException e) {
				String msg = SkinPropertiesConstants.KEYWINDOW_POSITION +
						" in .skin.properties is not numeric. : " + position;
				logger.warning(msg);

				skinProperties.remove(SkinPropertiesConstants.KEYWINDOW_POSITION);
			}
		}
	}

	public static void validateSkinConfigProperties(
			Properties skinConfigProperties) throws ConfigException {
		if (null == skinConfigProperties || 0 == skinConfigProperties.size()) {
			return;
		}

		// TODO: HEART_BEAT_IGNORE, LOG_LEVEL
	}

	public void saveSkinProperties() {
		File file = new File(skinPropertiesFilePath);

		if (!file.exists()) {
			try {
				if (!file.createNewFile()) {
					return;
				}
			} catch (IOException e) {
				logger.log(Level.SEVERE, "Fail to create skin properties file.", e);
				return;
			}
		}

		FileOutputStream fos = null;

		try {
			fos = new FileOutputStream(file);

			skinProperties.store(fos, "Automatically generated by emulator skin.");
		} catch (IOException e) {
			logger.log(Level.SEVERE, e.getMessage(), e);
		} finally {
			IOUtil.close(fos);
		}
	}

	public EmulatorUI getDbiContents() {
		return dbiContents;
	}

	/* arguments */
	public String getArg(String argKey) { /* string */
		return args.get(argKey);
	}

	public String getArg(String argKey, String defaultValue) {
		String arg = args.get(argKey);
		if (StringUtil.isEmpty(arg)) {
			return defaultValue;
		} else {
			return arg;
		}
	}

	public int getArgInt(String argKey) { /* integer */
		String arg = args.get(argKey);
		if (StringUtil.isEmpty(arg)) {
			return 0;
		}
		return Integer.parseInt(arg);
	}

	public int getArgInt(String argKey, int defaultValue) {
		String arg = args.get(argKey);
		if (StringUtil.isEmpty(arg)) {
			return defaultValue;
		}
		return Integer.parseInt(arg);
	}

	public boolean getArgBoolean(String argKey) { /* boolean */
		String arg = args.get(argKey);
		return Boolean.parseBoolean(arg);
	}

	public boolean getArgBoolean(String argKey, boolean defaultValue) {
		String arg = args.get(argKey);
		if (StringUtil.isEmpty(arg)) {
			return defaultValue;
		}
		return Boolean.parseBoolean(arg);
	}

	public int getValidResolutionWidth() {
		final int storedValue = getArgInt(ArgsConstants.RESOLUTION_WIDTH, 1);

		if (storedValue > 0) {
			return storedValue;
		} else {
			return 1;
		}
	}

	public int getValidResolutionHeight() {
		final int storedValue = getArgInt(ArgsConstants.RESOLUTION_HEIGHT, 1);

		if (storedValue > 0) {
			return storedValue;
		} else {
			return 1;
		}
	}

	/* java properties */
	private void setProperty(Properties properties, String key, String value) {
		properties.put(key, value);
	}

	private void setProperty(Properties properties, String key, int value) {
		properties.put(key, Integer.toString(value));
	}

	private String getProperty(Properties properties, String key) {
		return properties.getProperty(key);
	}

	private String getProperty(Properties properties, String key,
			String defaultValue) {
		String property = properties.getProperty(key);
		if (StringUtil.isEmpty(property)) {
			return defaultValue;
		}
		return property;
	}

	private int getPropertyInt(Properties properties, String key) {
		return Integer.parseInt(properties.getProperty(key));
	}

	private int getPropertyInt(Properties properties, String key,
			int defaultValue) {
		String property = properties.getProperty(key);
		if (StringUtil.isEmpty(property)) {
			return defaultValue;
		}
		return Integer.parseInt(property);
	}

	private short getPropertyShort(Properties properties, String key) {
		return Short.parseShort(properties.getProperty(key));
	}

	private short getPropertyShort(Properties properties, String key,
			short defaultValue) {
		String property = properties.getProperty(key);
		if (StringUtil.isEmpty(property)) {
			return defaultValue;
		}
		return Short.parseShort(property);
	}

	/* skin properties */
	public String getSkinProperty(String key) {
		return getProperty(skinProperties, key);
	}

	public String getSkinProperty(String key, String defaultValue) {
		return getProperty(skinProperties, key, defaultValue);
	}

	public int getSkinPropertyInt(String key) {
		return getPropertyInt(skinProperties, key);
	}

	public int getSkinPropertyInt(String key, int defaultValue) {
		return getPropertyInt(skinProperties, key, defaultValue);
	}

	public short getSkinPropertyShort(String key) {
		return getPropertyShort(skinProperties, key);
	}

	public short getSkinPropertyShort(String key, short defaultValue) {
		return getPropertyShort(skinProperties, key, defaultValue);
	}

	public void setSkinProperty(String key, String value) {
		setProperty(skinProperties, key, value);
	}

	public void setSkinProperty(String key, int value) {
		setProperty(skinProperties, key, value);
	}

	public int getValidWindowX() {
		int vmIndex = getArgInt(ArgsConstants.VM_BASE_PORT) % 100;
		final int storedValue = getSkinPropertyInt(SkinPropertiesConstants.WINDOW_X,
				EmulatorConfig.DEFAULT_WINDOW_X + vmIndex);

		Rectangle monitorBound = Display.getDefault().getBounds();
		logger.info("current display size : " + monitorBound);

		/* location correction */
		int xx = 0;
		if (storedValue < monitorBound.x) {
			xx = monitorBound.x;
			logger.info("WINDOW_X = " + storedValue + " -> " + xx);
		} else if (storedValue > monitorBound.x + monitorBound.width - 30) {
			xx = monitorBound.x + monitorBound.width - 100;
			logger.info("WINDOW_X = " + storedValue + " -> " + xx);
		} else {
			xx = storedValue;
			logger.info("WINDOW_X = " + xx);
		}

		return xx;
	}

	public int getValidWindowY() {
		int vmIndex = getArgInt(ArgsConstants.VM_BASE_PORT) % 100;
		final int storedValue = getSkinPropertyInt(SkinPropertiesConstants.WINDOW_Y,
				EmulatorConfig.DEFAULT_WINDOW_Y + vmIndex);

		Rectangle monitorBound = Display.getDefault().getBounds();
		logger.info("current display size : " + monitorBound);

		/* location correction */
		int yy = 0;
		if (storedValue < monitorBound.y) {
			yy = monitorBound.y;
			logger.info("WINDOW_Y = " + storedValue + " -> " + yy);
		} else if (storedValue > monitorBound.y + monitorBound.height - 30) {
			yy = monitorBound.y + monitorBound.height - 100;
			logger.info("WINDOW_Y = " + storedValue + " -> " + yy);
		} else {
			yy = storedValue;
			logger.info("WINDOW_Y = " + yy);
		}

		return yy;
	}

	public int getValidScale() {
		final int storedScale = getSkinPropertyInt(
				SkinPropertiesConstants.WINDOW_SCALE, DEFAULT_WINDOW_SCALE);

		if (storedScale >= MIN_SCALE_FACTOR &&
				storedScale <= MAX_SCALE_FACTOR) { /* percentage */
			return storedScale;
		} else {
			return DEFAULT_WINDOW_SCALE;
		}
	}

	/* config properties */
	public String getConfigProperty(String key) {
		return getProperty(configProperties, key);
	}

	public String getConfigProperty(String key, String defaultValue) {
		return getProperty(configProperties, key, defaultValue);
	}

	public int getConfigPropertyInt(String key, int defaultValue) {
		return getPropertyInt(configProperties, key, defaultValue);
	}
}
