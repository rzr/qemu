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

package org.tizen.emulator.skin.dialog;

import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.Map.Entry;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
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

/**
 * 
 *
 */
public class DetailInfoDialog extends SkinDialog {

	public final static int DETAIL_INFO_WAIT_INTERVAL = 1; // milli-seconds
	public final static int DETAIL_INFO_WAIT_LIMIT = 3000; // milli-seconds
	public final static String DATA_DELIMITER = "#";

	private Logger logger = SkinLogger.getSkinLogger( DetailInfoDialog.class ).getLogger();

	private SocketCommunicator communicator;
	private EmulatorConfig config;

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

		Table table = new Table( composite, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION );
		table.setHeaderVisible( true );
		table.setLinesVisible( true );

		TableColumn[] column = new TableColumn[2];

		column[0] = new TableColumn( table, SWT.LEFT );
		column[0].setText( "Name" );

		column[1] = new TableColumn( table, SWT.LEFT );
		column[1].setText( "Value" );

		int index = 0;

		LinkedHashMap<String, String> refinedData = composeAndParseData( infoData );
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

		return composite;

	}

	@Override
	protected void setShellSize() {
		if( SkinUtil.isLinuxPlatform() ) {
			shell.setSize( (int) ( 380 * 1.618 ), 380 );
		}else {
			shell.setSize( (int) ( 350 * 1.618 ), 350 );
		}
	}

	private String queryData() {

		String infoData = null;

		communicator.sendToQEMU( SendCommand.DETAIL_INFO, null, true );
		DataTranfer dataTranfer = communicator.getDataTranfer();

		int count = 0;
		boolean isFail = false;
		byte[] receivedData = null;
		int limitCount = DETAIL_INFO_WAIT_LIMIT / DETAIL_INFO_WAIT_INTERVAL;

		synchronized ( dataTranfer ) {

			while ( dataTranfer.isTransferState() ) {

				if ( limitCount < count ) {
					isFail = true;
					break;
				}

				try {
					dataTranfer.wait( DETAIL_INFO_WAIT_INTERVAL );
				} catch ( InterruptedException e ) {
					logger.log( Level.SEVERE, e.getMessage(), e );
				}

				count++;
				logger.info( "wait detail info data... count:" + count );

			}

			receivedData = dataTranfer.getReceivedData();

		}

		if ( isFail ) {

			logger.severe( "Fail to get detail info from server." );
			SkinUtil.openMessage( shell, null, "Internal error.", SWT.ICON_ERROR, config );

		} else {

			if ( null != receivedData ) {
				infoData = new String( receivedData );
			} else {
				logger.severe( "Received detail info is null" );
				SkinUtil.openMessage( shell, null, "Internal error.", SWT.ICON_ERROR, config );
			}

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

		if ( SkinUtil.isLinuxPlatform() ) {
			hwVirtualCompare = "-enable-kvm";
		} else if ( SkinUtil.isWindowsPlatform() ) {
			hwVirtualCompare = "-enable-hax";
		}
		
		String[] split = infoData.split( DATA_DELIMITER );

		for ( int i = 0; i < split.length; i++ ) {

			if ( 0 == i ) {

				String exec = split[i].trim().toLowerCase();
				if( SkinUtil.isWindowsPlatform() ) {
					if( 4 <= exec.length() ) {
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

						// arg : file=/home/xxx/.tizen_vms/x86/xxx/emulimg-emulator.x86
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
							if( idx + 7 <= arg.length() ) {
								
								dpi = arg.substring( idx, idx + 7 ); // end index is not 8, remove last '0'
								
								String[] sp = dpi.split( "=" );
								if ( 1 < sp.length ) {
									dpi = sp[1];
								}
								
							}
						}

					} else if ( hwVirtualCompare.equals( arg ) ) {
						isHwVirtual = true;
					}

				}

			}

		}

		LinkedHashMap<String, String> result = new LinkedHashMap<String, String>();

		result.put( "Name", SkinUtil.getVmName( config ) );
		result.put( "CPU", cpu );

		int width = config.getArgInt( ArgsConstants.RESOLUTION_WIDTH );
		int height = config.getArgInt( ArgsConstants.RESOLUTION_HEIGHT );
		result.put( "Display Resolution", width + "x" + height );
		result.put( "Display Density", dpi );

		if ( StringUtil.isEmpty( sdPath ) ) {
			result.put( "SD Card", "Not Supported" );
			result.put( "SD Card Path", "None" );
		} else {
			result.put( "SD Card", "Supported" );
			result.put( "SD Card Path", sdPath );
		}

		result.put( "RAM Size", ram );

		if( SkinUtil.isLinuxPlatform() ) {
			if ( StringUtil.isEmpty( sharedPath ) ) {
				result.put( "File Sharing", "Not Supported" );
				result.put( "File Shared Path", "None" );
			}else {
				result.put( "File Sharing", "Supported" );
				result.put( "File Shared Path", sharedPath );
			}
		}

		if( isHwVirtual ) {
			result.put( "HW Virtualization", "Supported" );
		}else {
			result.put( "HW Virtualization", "Not Supported" );
		}
		
		if ( StringUtil.isEmpty( imagePath ) ) {
			result.put( "Image Path", "Not identified" );			
		}else {
			result.put( "Image Path", imagePath );			
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

}
