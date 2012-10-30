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

package org.tizen.emulator.skin.config;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Map;
import java.util.Properties;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Display;
import org.tizen.emulator.skin.comm.ICommunicator.RotationInfo;
import org.tizen.emulator.skin.comm.ICommunicator.Scale;
import org.tizen.emulator.skin.dbi.EmulatorUI;
import org.tizen.emulator.skin.exception.ConfigException;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.log.SkinLogger.SkinLogLevel;
import org.tizen.emulator.skin.util.IOUtil;
import org.tizen.emulator.skin.util.StringUtil;


/**
 * 
 *
 */
public class EmulatorConfig {

	private static Logger logger = SkinLogger.getSkinLogger( EmulatorConfig.class ).getLogger();

	public static final int DEFAULT_WINDOW_SCALE = Scale.SCALE_50.value();
	public static final short DEFAULT_WINDOW_ROTATION = RotationInfo.PORTRAIT.id();
	public static final int DEFAULT_WINDOW_X = 50;
	public static final int DEFAULT_WINDOW_Y = 50;
	public static final SkinLogLevel DEFAULT_LOG_LEVEL = SkinLogLevel.DEBUG;

	public interface ArgsConstants {
		public static final String UID = "uid";
		public static final String SERVER_PORT = "svr.port";
		public static final String RESOLUTION_WIDTH = "width";
		public static final String RESOLUTION_HEIGHT = "height";
		public static final String TEST_HEART_BEAT_IGNORE = "test.hb.ignore";
		public static final String VM_PATH = "vm.path";
		public static final String LOG_LEVEL = "log.level";
		public static final String NET_BASE_PORT = "net.baseport";
		public static final String SKIN_PATH = "skin.path";
	}

	public interface SkinInfoConstants {
		public static final String SKIN_NAME = "skin.name";
		public static final String RESOLUTION_WIDTH = "resolution.width";
		public static final String RESOLUTION_HEIGHT = "resolution.height";
	}

	public interface SkinPropertiesConstants {
		public static final String WINDOW_X = "window.x";
		public static final String WINDOW_Y = "window.y";
		public static final String WINDOW_ROTATION = "window.rotate";
		public static final String WINDOW_SCALE = "window.scale";
		public static final String WINDOW_ONTOP = "window.ontop"; // always on top
	}

	public interface ConfigPropertiesConstants {
		public static final String TEST_HEART_BEAT_IGNORE = "test.hb.ignore";
		public static final String LOG_LEVEL = "log.level";
	}

	private Map<String, String> args;
	private EmulatorUI dbiContents;
	private Properties skinProperties;
	private Properties configProperties;
	private String skinPropertiesFilePath;

	public EmulatorConfig( Map<String, String> args, EmulatorUI dbiContents, Properties skinProperties,
			String skinPropertiesFilePath, Properties configProperties ) {
		this.args = args;
		this.dbiContents = dbiContents;
		this.skinProperties = skinProperties;
		this.skinPropertiesFilePath = skinPropertiesFilePath;
		this.configProperties = configProperties;
		if ( null == configProperties ) {
			this.configProperties = new Properties();
		}
	}

	public static void validateArgs(Map<String, String> args) throws ConfigException {
		if (null == args) {
			return;
		}

		if (args.containsKey(ArgsConstants.UID)) {
			String uid = args.get(ArgsConstants.UID);
			try {
				Integer.parseInt(uid);
			} catch (NumberFormatException e) {
				String msg = ArgsConstants.UID + " argument is not numeric. : " + uid;
				throw new ConfigException(msg);
			}
		}

		if (args.containsKey(ArgsConstants.SERVER_PORT)) {
			String serverPort = args.get(ArgsConstants.SERVER_PORT);
			try {
				Integer.parseInt(serverPort);
			} catch (NumberFormatException e) {
				String msg = ArgsConstants.SERVER_PORT + " argument is not numeric. : " + serverPort;
				throw new ConfigException(msg);
			}
		} else {
			String msg = ArgsConstants.SERVER_PORT + " is required argument.";
			throw new ConfigException(msg);
		}

		if (args.containsKey(ArgsConstants.RESOLUTION_WIDTH)) {
			String width = args.get(ArgsConstants.RESOLUTION_WIDTH);
			try {
				Integer.parseInt(width);
			} catch (NumberFormatException e) {
				String msg = ArgsConstants.RESOLUTION_WIDTH + " argument is not numeric. : " + width;
				throw new ConfigException(msg);
			}
		} else {
			String msg = ArgsConstants.RESOLUTION_WIDTH + " is required argument.";
			throw new ConfigException(msg);
		}

		if (args.containsKey(ArgsConstants.RESOLUTION_HEIGHT)) {
			String height = args.get(ArgsConstants.RESOLUTION_HEIGHT);
			try {
				Integer.parseInt(height);
			} catch (NumberFormatException e) {
				String msg = ArgsConstants.RESOLUTION_HEIGHT + " argument is not numeric. : " + height;
				throw new ConfigException(msg);
			}
		} else {
			String msg = ArgsConstants.RESOLUTION_HEIGHT + " is required argument.";
			throw new ConfigException(msg);
		}
	}

	public static void validateSkinProperties( Properties skinProperties ) throws ConfigException {
		if ( null == skinProperties || 0 == skinProperties.size() ) {
			return;
		}

		Rectangle monitorBound = Display.getDefault().getBounds();
		logger.info("current display size : " + monitorBound);

		if( skinProperties.containsKey( SkinPropertiesConstants.WINDOW_X ) ) {
			String x = skinProperties.getProperty( SkinPropertiesConstants.WINDOW_X );
			int xx = 0;

			try {
				xx = Integer.parseInt( x );
			} catch ( NumberFormatException e ) {
				String msg = SkinPropertiesConstants.WINDOW_X + " in .skin.properties is not numeric. : " + x;
				throw new ConfigException( msg );
			}

			//location correction
			if (xx < monitorBound.x) {
				int correction = monitorBound.x;
				logger.info("WINDOW_X = " + xx + ", set to " + correction);
				xx = correction;
			} else if (xx > monitorBound.x + monitorBound.width - 30) {
				int correction = monitorBound.x + monitorBound.width - 100;
				logger.info("WINDOW_X = " + xx + ", set to " + correction);
				xx = correction;
			} else {
				logger.info("WINDOW_X = " + xx);
			}

			skinProperties.setProperty(SkinPropertiesConstants.WINDOW_X, "" + xx);
		}

		if( skinProperties.containsKey( SkinPropertiesConstants.WINDOW_Y ) ) {
			String y = skinProperties.getProperty( SkinPropertiesConstants.WINDOW_Y );
			int yy = 0;

			try {
				yy = Integer.parseInt( y );
			} catch ( NumberFormatException e ) {
				String msg = SkinPropertiesConstants.WINDOW_Y + " in .skin.properties is not numeric. : " + y;
				throw new ConfigException( msg );
			}

			//location correction
			if (yy < monitorBound.y) {
				int correction = monitorBound.y;
				logger.info("WINDOW_Y = " + yy + ", set to " + correction);
				yy = correction;
			} else if (yy > monitorBound.y + monitorBound.height - 30) {
				int correction = monitorBound.y + monitorBound.height - 100;
				logger.info("WINDOW_Y = " + yy + ", set to " + correction);
				yy = correction;
			} else {
				logger.info("WINDOW_Y = " + yy);
			}

			skinProperties.setProperty(SkinPropertiesConstants.WINDOW_Y, "" + yy);
		}

		if( skinProperties.containsKey( SkinPropertiesConstants.WINDOW_ROTATION ) ) {
			String rotation = skinProperties.getProperty( SkinPropertiesConstants.WINDOW_ROTATION );
			try {
				Integer.parseInt( rotation );
			} catch ( NumberFormatException e ) {
				String msg = SkinPropertiesConstants.WINDOW_ROTATION + " in .skin.properties is not numeric. : " + rotation;
				throw new ConfigException( msg );
			}
		}

		if( skinProperties.containsKey( SkinPropertiesConstants.WINDOW_SCALE ) ) {
			String scale = skinProperties.getProperty( SkinPropertiesConstants.WINDOW_SCALE );
			try {
				Integer.parseInt( scale );
			} catch ( NumberFormatException e ) {
				String msg = SkinPropertiesConstants.WINDOW_SCALE + " in .skin.properties is not numeric. : " + scale;
				throw new ConfigException( msg );
			}
		}

	}

	public static void validateSkinConfigProperties( Properties skinConfigProperties ) throws ConfigException {
		if ( null == skinConfigProperties || 0 == skinConfigProperties.size() ) {
			return;
		}
	}

	public void saveSkinProperties() {

		File file = new File( skinPropertiesFilePath );

		if ( !file.exists() ) {

			try {
				if ( !file.createNewFile() ) {
					return;
				}
			} catch ( IOException e ) {
				logger.log( Level.SEVERE, "Fail to create skin properties file.", e );
				return;
			}

		}

		FileOutputStream fos = null;

		try {

			fos = new FileOutputStream( file );
			skinProperties.store( fos, "Automatically generated by emulator skin." );

		} catch ( IOException e ) {
			logger.log( Level.SEVERE, e.getMessage(), e );
		} finally {
			IOUtil.close( fos );
		}

	}

	public EmulatorUI getDbiContents() {
		return dbiContents;
	}

	public String getArg( String argKey ) {
		return args.get( argKey );
	}

	public String getArg( String argKey, String defaultValue ) {
		String arg = args.get( argKey );
		if ( StringUtil.isEmpty( arg ) ) {
			return defaultValue;
		} else {
			return arg;
		}
	}

	public int getArgInt( String argKey ) {
		String arg = args.get( argKey );
		if ( StringUtil.isEmpty( arg ) ) {
			return 0;
		}
		return Integer.parseInt( arg );
	}

	public int getArgInt( String argKey, int defaultValue ) {
		String arg = args.get( argKey );
		if ( StringUtil.isEmpty( arg ) ) {
			return defaultValue;
		}
		return Integer.parseInt( arg );
	}

	public boolean getArgBoolean( String argKey ) {
		String arg = args.get( argKey );
		return Boolean.parseBoolean( arg );
	}

	private String getProperty( Properties properties, String key ) {
		return properties.getProperty( key );
	}

	private String getProperty( Properties properties, String key, String defaultValue ) {
		String property = properties.getProperty( key );
		if ( StringUtil.isEmpty( property ) ) {
			return defaultValue;
		}
		return property;
	}

	private int getPropertyInt( Properties properties, String key ) {
		return Integer.parseInt( properties.getProperty( key ) );
	}

	private int getPropertyInt( Properties properties, String key, int defaultValue ) {
		String property = properties.getProperty( key );
		if ( StringUtil.isEmpty( property ) ) {
			return defaultValue;
		}
		return Integer.parseInt( property );
	}

	private short getPropertyShort( Properties properties, String key ) {
		return Short.parseShort( properties.getProperty( key ) );
	}

	private short getPropertyShort( Properties properties, String key, short defaultValue ) {
		String property = properties.getProperty( key );
		if ( StringUtil.isEmpty( property ) ) {
			return defaultValue;
		}
		return Short.parseShort( property );
	}

	private void setProperty( Properties properties, String key, String value ) {
		properties.put( key, value );
	}

	private void setProperty( Properties properties, String key, int value ) {
		properties.put( key, Integer.toString( value ) );
	}

	// skin properties //

	public String getSkinProperty( String key ) {
		return getProperty( skinProperties, key );
	}

	public String getSkinProperty( String key, String defaultValue ) {
		return getProperty( skinProperties, key, defaultValue );
	}

	public int getSkinPropertyInt( String key ) {
		return getPropertyInt( skinProperties, key );
	}

	public int getSkinPropertyInt( String key, int defaultValue ) {
		return getPropertyInt( skinProperties, key, defaultValue );
	}

	public short getSkinPropertyShort( String key ) {
		return getPropertyShort( skinProperties, key );
	}

	public short getSkinPropertyShort( String key, short defaultValue ) {
		return getPropertyShort( skinProperties, key, defaultValue );
	}

	public void setSkinProperty( String key, String value ) {
		setProperty( skinProperties, key, value );
	}

	public void setSkinProperty( String key, int value ) {
		setProperty( skinProperties, key, value );
	}

	// config properties //

	public String getConfigProperty( String key ) {
		return getProperty( configProperties, key );
	}

	public String getConfigProperty( String key, String defaultValue ) {
		return getProperty( configProperties, key, defaultValue );
	}

	public int getConfigPropertyInt( String key, int defaultValue ) {
		return getPropertyInt( configProperties, key, defaultValue );
	}

}