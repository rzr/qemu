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

package org.tizen.emulator.skin;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.Socket;
import java.util.HashMap;
import java.util.Map;
import java.util.Properties;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.MessageBox;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.comm.sock.SocketCommunicator;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.config.EmulatorConfig.ConfigPropertiesConstants;
import org.tizen.emulator.skin.config.EmulatorConfig.SkinInfoConstants;
import org.tizen.emulator.skin.config.EmulatorConfig.SkinPropertiesConstants;
import org.tizen.emulator.skin.dbi.EmulatorUI;
import org.tizen.emulator.skin.exception.JaxbException;
import org.tizen.emulator.skin.image.ImageRegistry;
import org.tizen.emulator.skin.info.SkinInformation;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.log.SkinLogger.SkinLogLevel;
import org.tizen.emulator.skin.util.IOUtil;
import org.tizen.emulator.skin.util.JaxbUtil;
import org.tizen.emulator.skin.util.StringUtil;
import org.tizen.emulator.skin.util.SwtUtil;

/**
 * 
 *
 */
public class EmulatorSkinMain {
	public static final String SKIN_INFO_FILE_NAME = "info.ini";
	public static final String SKIN_PROPERTIES_FILE_NAME = ".skin.properties";
	public static final String CONFIG_PROPERTIES_FILE_NAME = ".skinconfig.properties";
	public static final String DBI_FILE_NAME = "default.dbi";

	private static Logger logger;

	static {
		/* shared memory */
		if (SwtUtil.isMacPlatform()) {
		    System.loadLibrary("shared");
		}
	 }

	/**
	 * @param args
	 */
	public static void main( String[] args ) {

		String simpleMsg = getSimpleMsg(args);
		if (simpleMsg != null) {
			/* just show one message box. that's all. */
			Shell temp = new Shell(Display.getDefault());
			MessageBox messageBox = new MessageBox(temp, SWT.ICON_ERROR);
			messageBox.setText("Emulator");
			messageBox.setMessage(simpleMsg);
			messageBox.open();
			temp.dispose();

			System.exit(-1);
		}

		SocketCommunicator communicator = null;

		try {

			/* get vm path from startup argument */
			String vmPath = getVmPath( args );
			if ( StringUtil.isEmpty( vmPath ) ) {
				throw new IllegalArgumentException( ArgsConstants.VM_PATH + " in arguments is null." );
			}

			SkinLogger.init( SkinLogLevel.DEBUG, vmPath );

			logger = SkinLogger.getSkinLogger( EmulatorSkinMain.class ).getLogger();
			logger.info( "!!! Start Emualtor Skin !!!" );

			/* startup arguments parsing */
			Map<String, String> argsMap = parsArgs(args);

			/* emulator resolution */
			/*int resolutionW = Integer.parseInt(
					argsMap.get(ArgsConstants.RESOLUTION_WIDTH));
			int resolutionH = Integer.parseInt(
					argsMap.get(ArgsConstants.RESOLUTION_HEIGHT));*/

			/* get skin path from startup argument */
			String skinPath = ImageRegistry.getSkinPath(
					(String) argsMap.get(ArgsConstants.SKIN_PATH));

			/* set skin information */
			String skinInfoFilePath = skinPath + File.separator + SKIN_INFO_FILE_NAME;
			Properties skinInfoProperties = loadProperties(skinInfoFilePath, false);
			if (null == skinInfoProperties) {
				logger.severe("Fail to load skin information file.");

				Shell temp = new Shell(Display.getDefault());
				MessageBox messageBox = new MessageBox( temp, SWT.ICON_ERROR );
				messageBox.setText("Emulator");
				messageBox.setMessage("Fail to load \"" + SKIN_INFO_FILE_NAME + "\" file\n" +
						"Check if the file is corrupted or missing from the following path.\n" +
						skinPath);
				messageBox.open();
				temp.dispose();

				System.exit(-1);
			} else {
				logger.info("skin info:" + skinInfoProperties); //TODO:
			}

			boolean skinPhoneShape = true;
			String skinInfoResolutionW = skinInfoProperties.getProperty(SkinInfoConstants.RESOLUTION_WIDTH);
			String skinInfoResolutionH = skinInfoProperties.getProperty(SkinInfoConstants.RESOLUTION_HEIGHT);
			if (skinInfoResolutionW.equalsIgnoreCase("all") ||
					skinInfoResolutionH.equalsIgnoreCase("all")) {
				skinPhoneShape = false;
			}
			SkinInformation skinInfo = new SkinInformation(
					skinInfoProperties.getProperty(SkinInfoConstants.SKIN_NAME), skinPhoneShape);

			/* set emulator window skin property */
			String skinPropFilePath = vmPath + File.separator + SKIN_PROPERTIES_FILE_NAME;
			Properties skinProperties = loadProperties( skinPropFilePath, true );
			if ( null == skinProperties ) {
				logger.severe( "Fail to load skin properties file." );
				System.exit( -1 );
			}

			/* set emulator window config property */
			String configPropFilePath = vmPath + File.separator + CONFIG_PROPERTIES_FILE_NAME;
			Properties configProperties = loadProperties(configPropFilePath, false);

			/* able to use log file after loading properties */
			initLog(argsMap, configProperties);

			/* validation check */
			EmulatorConfig.validateArgs(argsMap);
			EmulatorConfig.validateSkinProperties(skinProperties);
			EmulatorConfig.validateSkinConfigProperties(configProperties);

			/* load dbi file */
			EmulatorUI dbiContents = loadDbi(skinPath);
			if ( null == dbiContents ) {
				logger.severe( "Fail to load dbi file." );

				Shell temp = new Shell(Display.getDefault());
				MessageBox messageBox = new MessageBox( temp, SWT.ICON_ERROR );
				messageBox.setText( "Emulator" );
				messageBox.setMessage( "Fail to load \"" + DBI_FILE_NAME + "\" file\n" +
						"Check if the file is corrupted or missing from the following path.\n" +
						skinPath);
				messageBox.open();
				temp.dispose();

				System.exit( -1 );
			}

			EmulatorConfig config = new EmulatorConfig( argsMap, dbiContents, skinProperties, skinPropFilePath,
					configProperties );

			/* load image resource */
			ImageRegistry.getInstance().initialize(config);

			/* load Always on Top value */
			String onTopVal = config.getSkinProperty(
					SkinPropertiesConstants.WINDOW_ONTOP, Boolean.FALSE.toString());
			boolean isOnTop = Boolean.parseBoolean(onTopVal);

			/* create skin */
			EmulatorSkin skin;
			if (SwtUtil.isMacPlatform()) {
				skin = new EmulatorShmSkin(config, skinInfo, isOnTop);
			} else { // linux & windows
				skin = new EmulatorSdlSkin(config, skinInfo, isOnTop);
			}

			long windowHandleId = skin.compose();

			int uid = config.getArgInt( ArgsConstants.UID );
			communicator = new SocketCommunicator( config, uid, windowHandleId, skin );

			skin.setCommunicator( communicator );

			Socket commSocket = communicator.getSocket();

			if ( null != commSocket ) {

				Runtime.getRuntime().addShutdownHook( new EmulatorShutdownhook( communicator ) );

				Thread communicatorThread = new Thread( communicator );
				communicatorThread.start();
				
//				SkinReopenPolicy reopenPolicy = skin.open();
//				
//				while( true ) {
//
//					if( null != reopenPolicy ) {
//						
//						if( reopenPolicy.isReopen() ) {
//							
//							EmulatorSkin reopenSkin = reopenPolicy.getReopenSkin();
//							logger.info( "Reopen skin dialog." );
//							reopenPolicy = reopenSkin.open();
//							
//						}else {
//							break;
//						}
//						
//					}else {
//						break;
//					}
//
//				}
				
				skin.open();
				
			} else {
				logger.severe( "CommSocket is null." );
			}

		} catch ( Throwable e ) {

			if ( null != logger ) {
				logger.log( Level.SEVERE, e.getMessage(), e );
				logger.warning( "Shutdown skin process !!!" );
			} else {
				e.printStackTrace();
				System.out.println( "Shutdown skin process !!!" );
			}
			
			if( null != communicator ) {
				communicator.terminate();
			}
			
		} finally {
			ImageRegistry.getInstance().dispose();
			Display.getDefault().close();
			SkinLogger.end();
		}

	}

	private static void initLog( Map<String, String> argsMap, Properties properties ) {

		String argLogLevel = argsMap.get( ArgsConstants.LOG_LEVEL );
		String configPropertyLogLevel = null;
		
		if( null != properties ) {
			configPropertyLogLevel = (String) properties.get( ConfigPropertiesConstants.LOG_LEVEL );
		}

		// default log level is debug.
		
		String logLevel = "";
		
		if( !StringUtil.isEmpty( argLogLevel ) ) {
			logLevel = argLogLevel;
		}else if( !StringUtil.isEmpty( configPropertyLogLevel ) ) {
			logLevel = configPropertyLogLevel;
		}else {
			logLevel = EmulatorConfig.DEFAULT_LOG_LEVEL.value();
		}
		
		SkinLogLevel skinLogLevel = EmulatorConfig.DEFAULT_LOG_LEVEL;
		
		SkinLogLevel[] values = SkinLogLevel.values();
		
		for ( SkinLogLevel level : values ) {
			if ( level.value().equalsIgnoreCase( logLevel ) ) {
				skinLogLevel = level;
				break;
			}
		}

		SkinLogger.setLevel( skinLogLevel.level() );
		
	}

	private static String getSimpleMsg( String[] args ) {

		for ( int i = 0; i < args.length; i++ ) {
			final String simple = "simple.msg";
			String arg = args[i];
			String[] split = arg.split( "=" );
			if ( 1 < split.length ) {
				if ( simple.equals( split[0].trim() ) ) {
					return split[1].trim();
				}
			}
		}

		return null;

	}

	private static String getVmPath( String[] args ) {

		for ( int i = 0; i < args.length; i++ ) {
			String arg = args[i];
			String[] split = arg.split( "=" );
			if ( 1 < split.length ) {
				if ( ArgsConstants.VM_PATH.equals( split[0].trim() ) ) {
					return split[1].trim();
				}
			}
		}

		return null;

	}
	
	private static Map<String, String> parsArgs( String[] args ) {

		Map<String, String> map = new HashMap<String, String>();

		for ( int i = 0; i < args.length; i++ ) {
			String arg = args[i];
			logger.info( "arg[" + i + "] " + arg );
			String[] split = arg.split( "=" );

			if ( 1 < split.length ) {

				String argKey = split[0].trim();
				String argValue = split[1].trim();
				map.put( argKey, argValue );

			} else {
				logger.info( "sinlge argv:" + arg );
			}
		}

		logger.info( "================= argsMap =====================" );
		logger.info( map.toString() );
		logger.info( "===============================================" );

		return map;

	}

	private static EmulatorUI loadDbi(String skinPath) {
		String dbiPath = skinPath + File.separator + DBI_FILE_NAME;
		logger.info("load dbi file from " + dbiPath);

		FileInputStream fis = null;
		EmulatorUI emulatorUI = null;

		try {
			
			fis = new FileInputStream(dbiPath);
			logger.info( "============ dbi contents ============" );
			byte[] bytes = IOUtil.getBytes( fis );
			logger.info( new String( bytes, "UTF-8" ) );
			logger.info( "=======================================" );
			
			emulatorUI = JaxbUtil.unmarshal( bytes, EmulatorUI.class );
			
		} catch ( IOException e ) {
			logger.log( Level.SEVERE, e.getMessage(), e );
		} catch ( JaxbException e ) {
			logger.log( Level.SEVERE, e.getMessage(), e );
		} finally {
			IOUtil.close( fis );
		}

		return emulatorUI;

	}

	private static Properties loadProperties( String filePath, boolean create ) {

		FileInputStream fis = null;
		Properties properties = null;

		try {

			File file = new File( filePath );
			
			if( create ) {
				
				if ( !file.exists() ) {
					if ( !file.createNewFile() ) {
						logger.severe( "Fail to create new " + filePath + " property file." );
						return null;
					}
				}
				
				fis = new FileInputStream( filePath );
				properties = new Properties();
				properties.load( fis );
				
			}else {
				
				if ( file.exists() ) {

					fis = new FileInputStream( filePath );
					properties = new Properties();
					properties.load( fis );
				}

			}

			logger.info( "load properties file : " + filePath );

		} catch ( IOException e ) {
			logger.log( Level.SEVERE, "Fail to load skin properties file.", e );
		} finally {
			IOUtil.close( fis );
		}

		return properties;

	}
	
}
