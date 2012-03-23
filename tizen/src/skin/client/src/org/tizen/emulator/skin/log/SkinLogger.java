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

package org.tizen.emulator.skin.log;

import java.io.File;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.HashMap;
import java.util.Map;
import java.util.logging.FileHandler;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.logging.SimpleFormatter;

import org.tizen.emulator.skin.util.StringUtil;

/**
 * 
 *
 */
public class SkinLogger {
	
	public static final String LOG_FOLDER = "logs";
	
	public enum SkinLogLevel {
		
		ERROR(Level.SEVERE, "error"),
		WARN(Level.WARNING, "warn"),
		DEBUG(Level.INFO, "debug"),
		TRACE(Level.FINE, "trace");
		
		private Level level;
		private String value;
		private SkinLogLevel( Level level, String value ) {
			this.level = level;
			this.value = value;
		}
		public Level level() {
			return this.level;
		}
		public String value() {
			return this.value;
		}
	}
	
	private static final String FILE_NAME = "emulator-skin.log";
	
	private static FileHandler fileHandler;
	private static boolean isInit;
	private static Map<Class<?>, SkinLogger> loggerMap = new HashMap<Class<?>, SkinLogger>();
	
	private Logger logger;
	
	private SkinLogger( Logger logger ) {
		this.logger = logger;
	}
	
	public Logger getLogger() {
		return this.logger;
	}
	
	public static void init( SkinLogLevel logLevel, String filePath ) {
		
		if( !isInit ) {
			
			isInit = true;
			
			String path = "";
			
			if( !StringUtil.isEmpty( filePath ) ) {
				path = filePath + File.separator;
			}

			File dir = new File( path + LOG_FOLDER );
			dir.mkdir();
			
			File file = new File( dir + File.separator + FILE_NAME );
			if( !file.exists() ) {
				try {
					if( !file.createNewFile() ) {
						System.err.println( "[SkinLog:error]Cannot create skin log file. path:" + file.getAbsolutePath() );
						System.exit( -1 );
						return;
					}
				} catch ( IOException e ) {
					e.printStackTrace();
					System.exit( -1 );
					return;
				}
			}

			try {
				System.out.println( "[SkinLog]log file path:" + file.getAbsolutePath() );
				fileHandler = new FileHandler( file.getAbsolutePath(), false );
			} catch ( SecurityException e1 ) {
				e1.printStackTrace();
			} catch ( IOException e1 ) {
				e1.printStackTrace();
			}

			try {
				fileHandler.setEncoding( "UTF-8" );
			} catch ( SecurityException e ) {
				e.printStackTrace();
			} catch ( UnsupportedEncodingException e ) {
				e.printStackTrace();
			}
			
			SimpleFormatter simpleFormatter = new SimpleFormatter();
			fileHandler.setFormatter( simpleFormatter );
			
			fileHandler.setLevel( logLevel.level() );
			
			System.out.println( "[SkinLog]SkinLogger level:" + logLevel.value() );
			
		}
		
	}
	
	public static void end() {
		loggerMap.clear();
	}
	
	public static <T> SkinLogger getSkinLogger( Class<T> clazz ) {
		
		String name = null;
		
		if( null == clazz ) {
			name = SkinLogger.class.getName();
		}else {
			name = clazz.getName();
		}
		
		SkinLogger skinLogger = loggerMap.get( clazz );
		
		if( null != skinLogger ) {
			return skinLogger;
		}else {
			
			Logger logger = Logger.getLogger( name );
			logger.addHandler( fileHandler );
			logger.setLevel( fileHandler.getLevel() );
			logger.setUseParentHandlers( false );
			
			SkinLogger sLogger = new SkinLogger( logger );
			loggerMap.put( clazz, sLogger );
			
			return sLogger;
			
		}
		
	}

}