/**
 * Emulator Skin Main
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

package org.tizen.emulator.skin;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.Socket;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.MessageBox;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.comm.sock.SocketCommunicator;
import org.tizen.emulator.skin.comm.sock.data.StartData;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.config.EmulatorConfig.ConfigPropertiesConstants;
import org.tizen.emulator.skin.config.EmulatorConfig.SkinInfoConstants;
import org.tizen.emulator.skin.config.EmulatorConfig.SkinPropertiesConstants;
import org.tizen.emulator.skin.dbi.EmulatorUI;
import org.tizen.emulator.skin.dbi.RotationType;
import org.tizen.emulator.skin.dbi.RotationsType;
import org.tizen.emulator.skin.exception.JaxbException;
import org.tizen.emulator.skin.image.ImageRegistry;
import org.tizen.emulator.skin.info.SkinInformation;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.log.SkinLogger.SkinLogLevel;
import org.tizen.emulator.skin.util.CocoaUtil;
import org.tizen.emulator.skin.util.IOUtil;
import org.tizen.emulator.skin.util.JaxbUtil;
import org.tizen.emulator.skin.util.SkinRotation;
import org.tizen.emulator.skin.util.StringUtil;
import org.tizen.emulator.skin.util.SwtUtil;

/**
 * 
 *
 */
public class EmulatorSkinMain {
	public static final String SKINS_FOLDER = "skins";
	public static final String DEFAULT_SKIN_FOLDER = "mobile-general-3btn";

	public static final String SKIN_INFO_FILE_NAME = "info.ini";
	public static final String SKIN_PROPERTIES_FILE_NAME = ".skin.properties";
	public static final String CONFIG_PROPERTIES_FILE_NAME = ".skinconfig.properties";
	public static final String DBI_FILE_NAME = "default.dbi";

	private static Logger logger;

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		/* make vm name and remove unused menu for macos-x */
		if (SwtUtil.isMacPlatform()) {
			System.setProperty("apple.laf.useScreenMenuBar", "true");
			System.setProperty("com.apple.mrj.application.apple.menu.about.name",
					"Emulator");
			Display display = Display.getDefault();
			display.syncExec(new Runnable() {
				public void run() {
					new CocoaUtil().removeTopMenuItems();
				}
			});
		}

		String simpleMsg = getSimpleMsg(args);
		if (simpleMsg != null) {
			/* just show one message box. that's all. */
			Shell temp = new Shell(Display.getDefault());
			MessageBox messageBox = new MessageBox(temp, SWT.ICON_ERROR);
			messageBox.setText("Emulator");
			messageBox.setMessage(simpleMsg);
			messageBox.open();
			temp.dispose();

			return;
		}

		SocketCommunicator communicator = null;

		try {
			/* get vm path from startup argument */
			String vmPath = getVmPath(args);
			if (StringUtil.isEmpty(vmPath)) {
				throw new IllegalArgumentException(
						ArgsConstants.VM_PATH + " in arguments is null.");
			}

			SkinLogger.init(SkinLogLevel.DEBUG, vmPath);

			logger = SkinLogger.getSkinLogger(EmulatorSkinMain.class).getLogger();
			logger.info("!!! Start Emualtor Skin !!!");

			logger.info("java.version : " + System.getProperty("java.version"));
			logger.info("java vendor : " + System.getProperty("java.vendor"));
			logger.info("vm version : " + System.getProperty("java.vm.version"));
			logger.info("vm vendor : " + System.getProperty("java.vm.vendor"));
			logger.info("vm name : " + System.getProperty("java.vm.name"));
			logger.info("os name : " + System.getProperty("os.name"));
			logger.info("os arch : " + System.getProperty("os.arch"));
			logger.info("os version : " + System.getProperty("os.version"));
			logger.info("swt platform : " + SWT.getPlatform());
			logger.info("swt version : " + SWT.getVersion());

			/* startup arguments parsing */
			Map<String, String> argsMap = parseArgs(args);
			EmulatorConfig.validateArgs(argsMap);

			/* get skin path from startup argument */
			String argSkinPath = (String) argsMap.get(ArgsConstants.SKIN_PATH);
			String skinPath = ".." + File.separator +
					SKINS_FOLDER + File.separator + DEFAULT_SKIN_FOLDER;

			File f = new File(argSkinPath);
			if (f.isDirectory() == false) {
				/* When emulator receive a invalid skin path,
				 emulator uses default skin path instead of it */
				logger.info("Emulator uses default skin path (" + skinPath +
						") instead of invalid skin path (" + argSkinPath + ").");
			} else {
				skinPath = argSkinPath;
			}

			/* set skin information */
			String skinInfoFilePath =
					skinPath + File.separator + SKIN_INFO_FILE_NAME;
			Properties skinInfoProperties = loadProperties(skinInfoFilePath, false);
			if (null == skinInfoProperties) {
				logger.severe("Fail to load skin information file.");

				Shell temp = new Shell(Display.getDefault());
				MessageBox messageBox = new MessageBox(temp, SWT.ICON_ERROR);
				messageBox.setText("Emulator");
				messageBox.setMessage("Fail to load \"" + SKIN_INFO_FILE_NAME + "\" file\n" +
						"Check if the file is corrupted or missing from the following path.\n" +
						skinPath);
				messageBox.open();
				temp.dispose();

				terminateImmediately(-1);
			}

			String skinInfoResolutionW =
					skinInfoProperties.getProperty(SkinInfoConstants.RESOLUTION_WIDTH);
			String skinInfoResolutionH =
					skinInfoProperties.getProperty(SkinInfoConstants.RESOLUTION_HEIGHT);

			logger.info("skin ini : " + SkinInfoConstants.SKIN_NAME + "=" +
					skinInfoProperties.getProperty(SkinInfoConstants.SKIN_NAME));
			logger.info("skin ini : " + SkinInfoConstants.RESOLUTION_WIDTH +
					"=" + skinInfoResolutionW);
			logger.info("skin ini : " + SkinInfoConstants.RESOLUTION_HEIGHT +
					"=" + skinInfoResolutionH);
			logger.info("skin ini : " + SkinInfoConstants.MANAGER_PRIORITY + "=" +
					skinInfoProperties.getProperty(SkinInfoConstants.MANAGER_PRIORITY));

			/* set emulator window skin property */
			String skinPropFilePath =
					vmPath + File.separator + SKIN_PROPERTIES_FILE_NAME;
			Properties skinProperties = loadProperties(skinPropFilePath, true);
			if (null == skinProperties) {
				logger.severe("Fail to load skin properties file.");
			}

			/* set emulator window config property */
			String configPropFilePath =
					vmPath + File.separator + CONFIG_PROPERTIES_FILE_NAME;
			Properties configProperties = loadProperties(configPropFilePath, false);
			EmulatorConfig.validateSkinConfigProperties(configProperties);

			/* able to use log file after loading properties */
			initLog(argsMap, configProperties);

			/* validation check */
			EmulatorConfig.validateSkinProperties(skinProperties);

			/* determine the layout */
			boolean isGeneralSkin = false;
			if (skinInfoResolutionW.equalsIgnoreCase("all") ||
					skinInfoResolutionH.equalsIgnoreCase("all")) {
				isGeneralSkin = true;
			}
			SkinInformation skinInfo = new SkinInformation(
					skinInfoProperties.getProperty(SkinInfoConstants.SKIN_NAME),
					skinPath, isGeneralSkin);

			/* load dbi file */
			EmulatorUI dbiContents = loadXMLForSkin(skinPath);
			if (null == dbiContents) {
				logger.severe("Fail to load dbi file.");

				Shell temp = new Shell(Display.getDefault());
				MessageBox messageBox = new MessageBox(temp, SWT.ICON_ERROR);
				messageBox.setText("Emulator");
				messageBox.setMessage("Fail to load \"" + DBI_FILE_NAME + "\" file\n" +
						"Check if the file is corrupted or missing from the following path.\n" +
						skinPath);
				messageBox.open();
				temp.dispose();

				terminateImmediately(-1);
			}
			logger.info("dbi version : " + dbiContents.getDbiVersion());

			/* collect configurations */
			EmulatorConfig config = new EmulatorConfig(argsMap,
					dbiContents, skinProperties, skinPropFilePath, configProperties);

			/* load image resource */
			ImageRegistry.getInstance().initialize(config, skinPath);

			/* load Always on Top value */
			String onTopVal = config.getSkinProperty(
					SkinPropertiesConstants.WINDOW_ONTOP, Boolean.FALSE.toString());
			boolean isOnTop = Boolean.parseBoolean(onTopVal);

			/* create a skin */
			EmulatorSkin skin = null;
			if (config.getArgBoolean(ArgsConstants.DISPLAY_SHM) == true) {
				logger.info("maru_shm"); /* shared framebuffer */

				skin = new EmulatorShmSkin(config, skinInfo, isOnTop);
			} else { /* linux & windows */
				logger.info("maru_sdl"); /* WINDOWID_hack */

				skin = new EmulatorSdlSkin(config, skinInfo, isOnTop);
			}

			/* create a qemu communicator */
			int uid = config.getArgInt(ArgsConstants.UID);
			communicator = new SocketCommunicator(config, uid, skin);
			skin.setCommunicator(communicator);

			/* initialize a skin layout */
			StartData startData = skin.initSkin();
			communicator.setInitialData(startData);

			Socket commSocket = communicator.getSocket();

			if (null != commSocket) {
				Runtime.getRuntime().addShutdownHook(
						new EmulatorShutdownhook(communicator));

				Thread communicatorThread =
						new Thread(communicator, "communicator");
				communicatorThread.start();

				/* Moves the receiver to the top of the drawing order for
				 the display on which it was created, marks it visible,
				 sets the focus and asks the window manager to make the
				 shell active */
				skin.open();
				
			} else {
				logger.severe("CommSocket is null.");
			}

		} catch (Throwable e) {
			if (null != logger) {
				logger.log(Level.SEVERE, e.getMessage(), e);
				logger.warning("Shutdown skin process !!!");
			} else {
				e.printStackTrace();
				System.out.println("Shutdown skin process !!!");
			}

			Shell temp = new Shell(Display.getDefault());
			MessageBox messageBox = new MessageBox(temp, SWT.ICON_ERROR);
			messageBox.setText("Emulator");
			messageBox.setMessage(e.getMessage());
			messageBox.open();
			temp.dispose();

			if (null != communicator) {
				communicator.terminate();
			} else {
				terminateImmediately(-1);
			}
		} finally {
			ImageRegistry.getInstance().dispose();
			Display.getDefault().close();
			SkinLogger.end();
		}
	}

	private static void initLog(
			Map<String, String> argsMap, Properties properties) {
		String argLogLevel = argsMap.get(ArgsConstants.LOG_LEVEL);
		String configPropertyLogLevel = null;
		
		if (null != properties) {
			configPropertyLogLevel =
					(String) properties.get(ConfigPropertiesConstants.LOG_LEVEL);
		}

		/* default log level is debug. */
		
		String logLevel = "";

		if (!StringUtil.isEmpty(argLogLevel)) {
			logLevel = argLogLevel;
		} else if (!StringUtil.isEmpty(configPropertyLogLevel)) {
			logLevel = configPropertyLogLevel;
		} else {
			logLevel = EmulatorConfig.DEFAULT_LOG_LEVEL.value();
		}

		SkinLogLevel skinLogLevel = EmulatorConfig.DEFAULT_LOG_LEVEL;

		SkinLogLevel[] values = SkinLogLevel.values();

		for (SkinLogLevel level : values) {
			if (level.value().equalsIgnoreCase(logLevel)) {
				skinLogLevel = level;
				break;
			}
		}

		SkinLogger.setLevel(skinLogLevel.level());
	}

	private static String getSimpleMsg(String[] args) {
		for (int i = 0; i < args.length; i++) {
			String arg = args[i];
			String[] split = arg.split("=");

			if (1 < split.length) {
				if (ArgsConstants.SIMPLE_MESSAGE.equals(
						split[0].trim())) {
					return split[1].trim();
				}
			}
		}

		return null;
	}

	private static String getVmPath(String[] args) {
		for (int i = 0; i < args.length; i++) {
			String arg = args[i];
			String[] split = arg.split("=");

			if (1 < split.length) {
				if (ArgsConstants.VM_PATH.equals(
						split[0].trim())) {
					return split[1].trim();
				}
			}
		}

		return null;
	}
	
	private static Map<String, String> parseArgs(String[] args) {
		Map<String, String> map = new HashMap<String, String>();

		for (int i = 0; i < args.length; i++) {
			String arg = args[i];
			logger.info("arg[" + i + "] " + arg);

			String[] split = arg.split("=");
			if (1 < split.length) {
				String argKey = split[0].trim();
				String argValue = split[1].trim();

				map.put(argKey, argValue);
			} else {
				logger.info("sinlge argv:" + arg);
			}
		}

		logger.info("================= argsMap =====================");
		logger.info(map.toString());
		logger.info("===============================================");

		return map;
	}

	private static EmulatorUI loadXMLForSkin(String skinPath) {
		String dbiPath = skinPath + File.separator + DBI_FILE_NAME;
		logger.info("load dbi file from " + dbiPath);

		FileInputStream fis = null;
		EmulatorUI emulatorUI = null;

		try {
			fis = new FileInputStream(dbiPath);
			logger.info("============ dbi contents ============");
			byte[] bytes = IOUtil.getBytes(fis);
			logger.info(new String(bytes, "UTF-8"));
			logger.info("=======================================");

			emulatorUI = JaxbUtil.unmarshal(bytes, EmulatorUI.class);

			/* register rotation info */
			RotationsType rotations = emulatorUI.getRotations();
			if (rotations != null) {
				List<RotationType> rotationList = rotations.getRotation();

				for (RotationType rotation : rotationList) {
					SkinRotation.put(rotation);
				}
			} else {
				logger.severe("Fail to loading rotations element from XML");
			}
		} catch (IOException e) {
			logger.log(Level.SEVERE, e.getMessage(), e);
		} catch (JaxbException e) {
			logger.log(Level.SEVERE, e.getMessage(), e);
		} finally {
			IOUtil.close(fis);
		}

		return emulatorUI;
	}

	private static Properties loadProperties(
			String filePath, boolean create) {
		FileInputStream fis = null;
		Properties properties = null;

		try {
			File file = new File(filePath);
			
			if (create == true) {
				if (file.exists() == false) {
					if (file.createNewFile() == false) {
						logger.severe(
								"Fail to create new " + filePath + " property file.");
						return null;
					}
				}

				fis = new FileInputStream(filePath);
				properties = new Properties();
				properties.load(fis);
			} else {
				if (file.exists() == true) {
					fis = new FileInputStream(filePath);
					properties = new Properties();
					properties.load(fis);
				}
			}

			logger.info("load properties file : " + filePath);
		} catch (IOException e) {
			logger.log(Level.SEVERE, "failed to load skin properties file", e);
		} finally {
			IOUtil.close(fis);
		}

		return properties;
	}

	public static void terminateImmediately(int exit) {
		if (logger != null) {
			logger.info("shutdown immediately! exit value : " + exit);
		}

		System.exit(exit);
	}
}
