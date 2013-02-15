/**
 * Display the emulator detail information
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
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

package org.tizen.emulator.skin.dialog;

import java.io.IOException;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.Map.Entry;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.program.Program;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.TableItem;
import org.tizen.emulator.skin.comm.ICommunicator.SendCommand;
import org.tizen.emulator.skin.comm.sock.SocketCommunicator;
import org.tizen.emulator.skin.comm.sock.SocketCommunicator.DataTranfer;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.util.SkinUtil;
import org.tizen.emulator.skin.util.StringUtil;
import org.tizen.emulator.skin.util.SwtUtil;

/**
 * 
 *
 */
public class DetailInfoDialog extends SkinDialog {

	public final static String DATA_DELIMITER = "#";
	public final static String KEY_SDCARD_PATH = "SD Card Path";
	public final static String KEY_FILESHARED_PATH = "File Shared Path";
	public final static String KEY_IMAGE_PATH = "Image Path";
	public final static String KEY_LOG_PATH = "Log Path";
	public final static String VALUE_NONE = "None";
	public final static String VALUE_SUPPORTED = "Supported";
	public final static String VALUE_NOTSUPPORTED = "Not Supported";

	private Logger logger = SkinLogger.getSkinLogger( DetailInfoDialog.class ).getLogger();

	private SocketCommunicator communicator;
	private EmulatorConfig config;
	private Table table;
	private LinkedHashMap<String, String> refinedData;

	public DetailInfoDialog( Shell parent, String emulatorName, SocketCommunicator communicator, EmulatorConfig config ) {
		super( parent, "Detail Info" + " - " + emulatorName, SWT.DIALOG_TRIM | SWT.APPLICATION_MODAL | SWT.RESIZE
				| SWT.MAX );
		this.communicator = communicator;
		this.config = config;
	}

	@Override
	protected Composite createArea( Composite parent ) {

		String infoData = queryData();
		if ( StringUtil.isEmpty( infoData ) ) {
			return null;
		}

		Composite composite = new Composite( parent, SWT.NONE );
		composite.setLayout( new FillLayout() );

		table = new Table( composite, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION );
		table.setHeaderVisible( true );
		table.setLinesVisible( true );

		TableColumn[] column = new TableColumn[2];

		column[0] = new TableColumn( table, SWT.LEFT );
		column[0].setText( "Feature" );

		column[1] = new TableColumn( table, SWT.LEFT );
		column[1].setText( "Value" );

		int index = 0;

		refinedData = composeAndParseData( infoData );
		Iterator<Entry<String, String>> iterator = refinedData.entrySet().iterator();

		while ( iterator.hasNext() ) {

			Entry<String, String> entry = iterator.next();

			TableItem tableItem = new TableItem( table, SWT.NONE, index );
			tableItem.setText( new String[] { entry.getKey(), entry.getValue() } );
			index++;

		}

		column[0].pack();
		column[1].pack();
		table.pack();

		/* browse the appropriate path when item is double clicked */
		table.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent event) {
				//do nothing
			}

			@Override
			public void widgetDefaultSelected(SelectionEvent event) {
				if (table.getSelectionCount() > 1) {
					return;
				}

				TableItem tableItem = ((TableItem)table.getSelection()[0]);
				String openPath = VALUE_NONE;

				if (tableItem.getText().compareTo(KEY_LOG_PATH) == 0) {
					openPath = refinedData.get(KEY_LOG_PATH);
				} else if (tableItem.getText().compareTo(KEY_IMAGE_PATH) == 0) {
					openPath = refinedData.get(KEY_IMAGE_PATH);
				} else if (tableItem.getText().compareTo(KEY_FILESHARED_PATH) == 0) {
					openPath = refinedData.get(KEY_FILESHARED_PATH);
				} else if (tableItem.getText().compareTo(KEY_SDCARD_PATH) == 0) {
					openPath = refinedData.get(KEY_SDCARD_PATH);
				}

				try {
					openPath = StringUtil.getCanonicalPath(openPath);
				} catch (IOException e) {
					logger.warning( "Invalid path" );
				}

				if (openPath.compareTo(VALUE_NONE) == 0 || openPath.compareTo("") == 0) {
					return;
				}

				Program.launch(openPath);

				/*ProcessBuilder procBrowser = new ProcessBuilder();

				if (SwtUtil.isLinuxPlatform()) {
					procBrowser.command("nautilus", "--browser", openPath);
				} else if (SwtUtil.isWindowsPlatform()) {
					procBrowser.command("explorer", "\"" + openPath + "\"");
				} else if (SwtUtil.isMacPlatform()) {
					//TODO:
					logger.warning( "not supported yet" );
				}

				if (procBrowser.command().isEmpty() == false) {
					try {
						procBrowser.start();
					} catch (Exception e) {
						logger.log( Level.SEVERE, e.getMessage(), e);
					}
				}*/

			}
		});

		return composite;

	}

	@Override
	protected void setShellSize() {
		if( SwtUtil.isLinuxPlatform() ) {
			shell.setSize( (int) ( 402 * 1.618 ), 402 );
		} else {
			shell.setSize( (int) ( 372 * 1.618 ), 372 );
		}
	}

	private String queryData() {

		String infoData = null;

		DataTranfer dataTranfer = communicator.sendToQEMU( SendCommand.DETAIL_INFO, null, true );
		byte[] receivedData = communicator.getReceivedData( dataTranfer );

		if ( null != receivedData ) {
			infoData = new String( receivedData );
		} else {
			logger.severe( "Fail to get detail info." );
			SkinUtil.openMessage( shell, null, "Fail to get detail info.", SWT.ICON_ERROR, config );
		}

		return infoData;

	}

	private LinkedHashMap<String, String> composeAndParseData( String infoData ) {

		logger.info( "Received infoData:" + infoData );

		String cpu = "";
		String ram = "";
		String dpi = "";
		String sdPath = "";
		String imagePath = "";
		boolean isFirstDrive = true;
		String sharedPath = "";
		boolean isHwVirtual = false;
		String hwVirtualCompare = "";
		String logPath = "";
		boolean isHaxError = false;
		
		if ( SwtUtil.isLinuxPlatform() ) {
			hwVirtualCompare = "-enable-kvm";
		} else if ( SwtUtil.isWindowsPlatform() ) {
			hwVirtualCompare = "-enable-hax";
		}
		
		String[] split = infoData.split( DATA_DELIMITER );

		for ( int i = 0; i < split.length; i++ ) {

			if ( 0 == i ) {

				String exec = split[i].trim().toLowerCase();
				if ( SwtUtil.isWindowsPlatform() ) {
					if ( 4 <= exec.length() ) {
						// remove '.exe' in Windows
						exec = exec.substring( 0, exec.length() - 4 );
					}
				}
				
				if ( exec.endsWith( "x86" ) ) {
					cpu = "x86";
				} else if ( exec.endsWith( "arm" ) ) {
					cpu = "ARM";
				}

			} else {

				if ( i + 1 <= split.length ) {

					String arg = split[i].trim();

					if ( "-m".equals( arg ) ) {

						ram = split[i + 1].trim();

					} else if ( "-drive".equals( arg ) ) {

						// arg : file=/home/xxx/tizen-sdk-data/emulator-vms/vms/xxx/emulimg-xxx.x86
						arg = split[i + 1].trim();

						if ( arg.startsWith( "file=" ) ) {

							String[] sp = arg.split( "," );
							String[] sp2 = sp[0].split( "=" );
							String drivePath = sp2[sp2.length - 1];

							if ( isFirstDrive ) {
								imagePath = drivePath;
								isFirstDrive = false;
							} else {
								sdPath = drivePath;
							}

						}

					} else if ( "-virtfs".equals( arg ) ) {

						// arg : local,path=/home/xxx/xxx/xxx,security_model=none,mount_tag=fileshare
						arg = split[i + 1].trim();
						String[] sp = arg.split( "," );

						if ( 1 < sp.length ) {
							int spIndex = sp[1].indexOf( "=" );
							sharedPath = sp[1].substring( spIndex + 1, sp[1].length() );
						}

					} else if ( "-append".equals( arg ) ) {

						arg = split[i + 1].trim();

						int idx = arg.indexOf( "dpi" );

						if ( -1 != idx ) {
							if ( idx + 7 <= arg.length() ) {
								
								dpi = arg.substring( idx, idx + 7 ); // end index is not 8, remove last '0'
								
								String[] sp = dpi.split( "=" );
								if ( 1 < sp.length ) {
									dpi = sp[1];
								}
								
							}
						}

					} else if ( hwVirtualCompare.equals( arg ) ) {
						isHwVirtual = true;
					} else if ( arg.startsWith("hax_error=") ) {
						String[] sp = arg.split( "=" );
						if ( 1 < sp.length ) {
							isHaxError = Boolean.parseBoolean( sp[1] );
						}
					} else if (arg.startsWith("log_path=")) {
						String[] sp = arg.split("=");
						if ( 1 < sp.length ) {
							logPath = sp[1];

							try {
								logPath = StringUtil.getCanonicalPath(logPath);
							} catch (IOException e) {
								logger.log(Level.SEVERE, e.getMessage(), e);
							}
							logger.info("log path = " + logPath); //without filename
						}
					}

				}

			}

		}

		LinkedHashMap<String, String> result = new LinkedHashMap<String, String>();

		/* Target name */
		result.put( "Name", SkinUtil.getVmName( config ) );

		/* CPU srchitecture */
		result.put( "CPU", cpu );

		/* Target display resolution */
		int width = config.getArgInt( ArgsConstants.RESOLUTION_WIDTH );
		int height = config.getArgInt( ArgsConstants.RESOLUTION_HEIGHT );
		result.put( "Display Resolution", width + "x" + height );

		/* DPI (dots per inch) */
		result.put( "Display Density", dpi );

		/* SD card path */
		if ( StringUtil.isEmpty( sdPath ) ) {
			result.put("SD Card", VALUE_NOTSUPPORTED);
			result.put(KEY_SDCARD_PATH, VALUE_NONE);
		} else {
			result.put("SD Card", VALUE_SUPPORTED);
			result.put(KEY_SDCARD_PATH, sdPath);
		}

		/* RAM size */
		result.put( "RAM Size", ram );

		/* Whether host file sharing is supported */
		if ( StringUtil.isEmpty( sharedPath ) ) {
			result.put("File Sharing", VALUE_NOTSUPPORTED);
			result.put(KEY_FILESHARED_PATH, VALUE_NONE);
		} else {
			result.put("File Sharing", VALUE_SUPPORTED);
			result.put(KEY_FILESHARED_PATH, sharedPath);
		}

		/* Whether hardware virtualization is supported */
		if ( isHwVirtual ) {
			if ( isHaxError ) {
				result.put( "HW Virtualization State", "Disable(insufficient memory for driver)" );
			} else {
				result.put( "HW Virtualization State", "Enable" );
			}
		} else {
			result.put( "HW Virtualization State", "Disable" );
		}

		/* Target image path */
		if ( StringUtil.isEmpty( imagePath ) ) {
			result.put(KEY_IMAGE_PATH, "Not identified");
		} else {
			result.put(KEY_IMAGE_PATH, imagePath);
		}

		/* Emulator log file path */
		if (StringUtil.isEmpty(logPath)) {
			result.put(KEY_LOG_PATH, VALUE_NONE);
		} else {
			result.put(KEY_LOG_PATH, logPath);
		}

		return result;

	}

	protected void createButtons( Composite parent ) {
		super.createButtons( parent );

		Button okButton = createButton( parent, OK );

		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.RIGHT;
		okButton.setLayoutData( gd );

		okButton.setFocus();

		okButton.addSelectionListener( new SelectionAdapter() {
			@Override
			public void widgetSelected( SelectionEvent e ) {
				DetailInfoDialog.this.shell.close();
			}
		} );

	};

	protected void close() {
		logger.info("close the detail info dialog");
	}
}
