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

import org.tizen.emulator.skin.comm.sock.SocketCommunicator;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.dbi.EmulatorUI;
import org.tizen.emulator.skin.exception.JaxbException;
import org.tizen.emulator.skin.image.ImageRegistry;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.log.SkinLogger.SkinLogLevel;
import org.tizen.emulator.skin.util.IOUtil;
import org.tizen.emulator.skin.util.JaxbUtil;
import org.tizen.emulator.skin.util.StringUtil;

/**
 * 
 *
 */
public class EmulatorSkinMain {

	public static final String PROPERTIES_FILE_NAME = ".skin.properties";
	public static final String DBI_FILE_NAME = "default.dbi";

	private static Logger logger;
	
	/**
	 * @param args
	 */
	public static void main( String[] args ) {
		
		try {
			
			initLog( args );

			Map<String, String> argsMap = parsArgs( args );

			int lcdWidth = Integer.parseInt( argsMap.get( ArgsConstants.RESOLUTION_WIDTH ) );
			int lcdHeight = Integer.parseInt( argsMap.get( ArgsConstants.RESOLUTION_HEIGHT ) );
			EmulatorUI dbiContents = loadDbi( lcdWidth, lcdHeight );
			if ( null == dbiContents ) {
				logger.severe( "Fail to load dbi file." );
				return;
			}

			String vmPath = (String) argsMap.get( ArgsConstants.VM_PATH );
			String propFilePath = vmPath + File.separator + PROPERTIES_FILE_NAME;
			Properties properties = loadProperties( propFilePath );
			if ( null == properties ) {
				logger.severe( "Fail to load properties file." );
				return;
			}

			EmulatorConfig config = new EmulatorConfig( argsMap, dbiContents, properties, propFilePath );

			EmulatorSkin skin = new EmulatorSkin( config );
			int windowHandleId = skin.compose();

			int uid = Integer.parseInt( config.getArg( ArgsConstants.UID ) );
			SocketCommunicator communicator = new SocketCommunicator( config, uid, windowHandleId, skin );

			skin.setCommunicator( communicator );

			Socket commSocket = communicator.getSocket();

			if ( null != commSocket ) {

				Runtime.getRuntime().addShutdownHook( new EmulatorShutdownhook( communicator, skin ) );

				Thread communicatorThread = new Thread( communicator );
				communicatorThread.start();

				skin.open();

			} else {
				logger.severe( "CommSocket is null." );
			}

		} catch ( Throwable e ) {

			if ( null != logger ) {
				logger.log( Level.SEVERE, e.getMessage(), e );
			} else {
				e.printStackTrace();
			}

		} finally {
			SkinLogger.end();
		}

	}

	private static void initLog( String[] args ) {

		String logLevel = "";
		String vmPath = "";
		
		for ( int i = 0; i < args.length; i++ ) {
			
			String[] split = args[i].split( "=" );
			
			if ( split[0].trim().equalsIgnoreCase( ArgsConstants.LOG_LEVEL ) ) {
				if ( !StringUtil.isEmpty( split[1].trim() ) ) {
					logLevel = split[1].trim();
				}
			}else if ( split[0].trim().equalsIgnoreCase( ArgsConstants.VM_PATH ) ) {
				vmPath = split[1].trim();
			}
			
		}

		SkinLogLevel skinLogLevel = SkinLogLevel.WARN;
		
		if( !StringUtil.isEmpty( logLevel ) ) {
			
			SkinLogLevel[] values = SkinLogLevel.values();
			
			for ( SkinLogLevel level : values ) {
				if ( level.value().equalsIgnoreCase( logLevel ) ) {
					skinLogLevel = level;
					break;
				}
			}
			
		}

		SkinLogger.init( skinLogLevel, vmPath );
		logger = SkinLogger.getSkinLogger( EmulatorSkinMain.class ).getLogger();

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
				logger.info( "argKey:" + argKey + "  argValue:" + argValue );
				map.put( argKey, argValue );

			} else {
				logger.info( "one argv:" + arg );
			}
		}

		logger.info( "========================================" );
		logger.info( "args:" + map );
		logger.info( "========================================" );

		return map;

	}

	private static EmulatorUI loadDbi( int lcdWidth, int lcdHeight ) {

		String skinPath = ImageRegistry.getSkinPath( lcdWidth, lcdHeight ) + File.separator + DBI_FILE_NAME;

		FileInputStream fis = null;
		EmulatorUI emulatorUI = null;

		try {

			fis = new FileInputStream( skinPath );
			
			emulatorUI = JaxbUtil.unmarshal( fis, EmulatorUI.class );

			fis = new FileInputStream( skinPath );
			logger.info( "============ dbi contents ============" );
			byte[] bytes = IOUtil.getBytes( fis );
			logger.info( new String( bytes, "UTF-8" ) );
			logger.info( "=======================================" );

		} catch ( IOException e ) {
			logger.log( Level.SEVERE, e.getMessage(), e );
		} catch ( JaxbException e ) {
			logger.log( Level.SEVERE, e.getMessage(), e );
		} finally {
			IOUtil.close( fis );
		}

		return emulatorUI;

	}

	private static Properties loadProperties( String filePath ) {

		FileInputStream fis = null;
		Properties properties = null;

		try {

			File file = new File( filePath );
			if ( !file.exists() ) {
				if ( !file.createNewFile() ) {
					logger.severe( "Fail to create new " + filePath + " property file." );
					return null;
				}
			}

			fis = new FileInputStream( filePath );
			properties = new Properties();
			properties.load( fis );

		} catch ( IOException e ) {
			logger.log( Level.SEVERE, e.getMessage(), e );
		} finally {
			IOUtil.close( fis );
		}

		return properties;

	}

}