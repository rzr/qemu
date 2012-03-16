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

import org.tizen.emulator.skin.comm.sock.SocketCommunicator;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.dbi.EmulatorUI;
import org.tizen.emulator.skin.exception.JaxbException;
import org.tizen.emulator.skin.image.ImageRegistry;
import org.tizen.emulator.skin.util.IOUtil;
import org.tizen.emulator.skin.util.JaxbUtil;



/**
 * 
 *
 */
public class EmulatorSkinMain {
	
	public static final String PROPERTIES_FILE_NAME = ".skin.properties";
	public static final String DBI_FILE_NAME = "default.dbi";
	
	/**
	 * @param args
	 */
	public static void main( String[] args ) {

		Map<String, String> argsMap = parsArgs( args );

		int lcdWidth = Integer.parseInt( argsMap.get( ArgsConstants.RESOLUTION_WIDTH ) );
		int lcdHeight = Integer.parseInt( argsMap.get( ArgsConstants.RESOLUTION_HEIGHT ) );
		EmulatorUI dbiContents = loadDbi( lcdWidth, lcdHeight );
		if ( null == dbiContents ) {
			System.out.println( "Fail to load dbi file." );
			return;
		}

		String vmPath = (String) argsMap.get( ArgsConstants.VM_PATH );
		String propFilePath = vmPath + File.separator + PROPERTIES_FILE_NAME;
		Properties properties = loadProperties( propFilePath );
		if ( null == properties ) {
			System.out.println( "Fail to load properties file." );
			return;
		}

		EmulatorConfig config = new EmulatorConfig( argsMap, dbiContents, properties, propFilePath );

		EmulatorSkin skin = new EmulatorSkin( config );
		int windowHandleId = skin.compose();

		int pid = Integer.parseInt( config.getArg( ArgsConstants.UID ) );
		SocketCommunicator communicator = new SocketCommunicator( config, pid, windowHandleId, skin );

		skin.setCommunicator( communicator );

		Socket commSocket = communicator.getSocket();

		if ( null != commSocket ) {

			Runtime.getRuntime().addShutdownHook( new EmulatorShutdownhook( communicator, skin ) );

			Thread communicatorThread = new Thread( communicator );
			communicatorThread.start();

			skin.open();

		} else {
			System.out.println( "CommSocket is null." );
		}

	}

	private static Map<String, String> parsArgs( String[] args ) {

		Map<String, String> map = new HashMap<String, String>();

		// TODO parse
		for ( int i = 0; i < args.length; i++ ) {
			String arg = args[i];
			System.out.println( "arg[" + i + "] " + arg );
			String[] split = arg.split( "=" );

			if ( 1 < split.length ) {

				String argKey = split[0];
				String argValue = split[1];
				System.out.println( "argKey:" + argKey + "  argValue:" + argValue );
				map.put( argKey, argValue );

			} else {
				System.out.println( "one argv:" + arg );
			}
		}

		map.put( ArgsConstants.EMULATOR_NAME, "emulator" );
		
		System.out.println( "========================================");
		System.out.println( "args:" + map );
		System.out.println( "========================================");
		
		return map;

	}

	private static EmulatorUI loadDbi( int lcdWidth, int lcdHeight ) {

		String skinPath = ImageRegistry.getSkinPath( lcdWidth, lcdHeight ) + File.separator + DBI_FILE_NAME;
		
		FileInputStream fis = null;
		EmulatorUI emulatorUI = null;

		try {

			fis = new FileInputStream( skinPath );

			emulatorUI = JaxbUtil.unmarshal( fis, EmulatorUI.class );

			// XXX
			fis = new FileInputStream( skinPath );
			System.out.println( "============ dbi contents ============" );
			byte[] bytes = IOUtil.getBytes( fis );
			System.out.println( new String( bytes, "UTF-8" ) );
			System.out.println( "=======================================" );

		} catch ( IOException e ) {
			e.printStackTrace();
		} catch ( JaxbException e ) {
			e.printStackTrace();
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
					System.out.println( "Fail to create new " + filePath + " property file." );
					return null;
				}
			}

			fis = new FileInputStream( filePath );
			properties = new Properties();
			properties.load( fis );

		} catch ( IOException e ) {
			e.printStackTrace();
		} finally {
			IOUtil.close( fis );
		}

		return properties;

	}

}
