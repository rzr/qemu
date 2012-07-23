/**
 * Emulator Skin Process
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
import java.io.IOException;
import java.lang.reflect.Array;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map.Entry;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.KeyListener;
import org.eclipse.swt.events.MenuDetectEvent;
import org.eclipse.swt.events.MenuDetectListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.MouseTrackAdapter;
import org.eclipse.swt.events.MouseTrackListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.comm.ICommunicator.KeyEventType;
import org.tizen.emulator.skin.comm.ICommunicator.MouseEventType;
import org.tizen.emulator.skin.comm.ICommunicator.Scale;
import org.tizen.emulator.skin.comm.ICommunicator.SendCommand;
import org.tizen.emulator.skin.comm.sock.SocketCommunicator;
import org.tizen.emulator.skin.comm.sock.data.BooleanData;
import org.tizen.emulator.skin.comm.sock.data.KeyEventData;
import org.tizen.emulator.skin.comm.sock.data.LcdStateData;
import org.tizen.emulator.skin.comm.sock.data.MouseEventData;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.config.EmulatorConfig.SkinPropertiesConstants;
import org.tizen.emulator.skin.dbi.ColorsType;
import org.tizen.emulator.skin.dbi.RgbType;
import org.tizen.emulator.skin.dbi.RotationType;
import org.tizen.emulator.skin.dialog.AboutDialog;
import org.tizen.emulator.skin.dialog.DetailInfoDialog;
import org.tizen.emulator.skin.exception.ScreenShotException;
import org.tizen.emulator.skin.image.ImageRegistry;
import org.tizen.emulator.skin.image.ImageRegistry.IconName;
import org.tizen.emulator.skin.image.ImageRegistry.ImageType;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.screenshot.ScreenShotDialog;
import org.tizen.emulator.skin.util.SkinRegion;
import org.tizen.emulator.skin.util.SkinRotation;
import org.tizen.emulator.skin.util.SkinUtil;
import org.tizen.emulator.skin.util.SwtUtil;

/**
 * 
 *
 */
public class EmulatorSkin {

	public class SkinReopenPolicy {

		private EmulatorSkin reopenSkin;
		private boolean reopen;

		private SkinReopenPolicy( EmulatorSkin reopenSkin, boolean reopen ) {
			this.reopenSkin = reopenSkin;
			this.reopen = reopen;
		}

		public EmulatorSkin getReopenSkin() {
			return reopenSkin;
		}

		public boolean isReopen() {
			return reopen;
		}

	}

	public static final String GTK_OS_CLASS = "org.eclipse.swt.internal.gtk.OS";
	public static final String WIN32_OS_CLASS = "org.eclipse.swt.internal.win32.OS";
	
	private Logger logger = SkinLogger.getSkinLogger( EmulatorSkin.class ).getLogger();

	private EmulatorConfig config;
	private Shell shell;
	private ImageRegistry imageRegistry;
	private Canvas lcdCanvas;
	private Image currentImage;
	private Image currentKeyPressedImage;
	private Color hoverColor;
	private boolean isDefaultHoverColor;

	private int currentScale;
	private short currentRotationId;
	private int currentAngle;
	private int currentLcdWidth;
	private int currentLcdHeight;
	private SkinRegion currentHoverRegion;
	private SkinRegion currentPressedRegion;

	private int pressedMouseX;
	private int pressedMouseY;
	private boolean isMousePressed;
	private boolean isDragStartedInLCD;
	private boolean isHoverState;
	private boolean isShutdownRequested;
	private boolean isAboutToReopen;
	private boolean isOnTop;
	private boolean isScreenShotOpened;
	private boolean isOnUsbKbd;

	private ScreenShotDialog screenShotDialog;

	private SocketCommunicator communicator;
	private long windowHandleId;

	private Listener shellCloseListener;
	private PaintListener shellPaintListener;
	private MouseTrackListener shellMouseTrackListener;
	private MouseMoveListener shellMouseMoveListener;
	private MouseListener shellMouseListener;

	//private DragDetectListener canvasDragDetectListener;
	private MouseMoveListener canvasMouseMoveListener;
	private MouseListener canvasMouseListener;
	private KeyListener canvasKeyListener;
	private MenuDetectListener canvasMenuDetectListener;

	private EmulatorSkin reopenSkin;
	
	/**
	 * @brief constructor
	 * @param config : configuration of emulator skin
	 * @param isOnTop : always on top flag
	*/
	protected EmulatorSkin( EmulatorConfig config, boolean isOnTop ) {
		this.config = config;
		this.isDefaultHoverColor = true;
		this.isOnTop = isOnTop;
		
		int style = SWT.NO_TRIM;
		this.shell = new Shell( Display.getDefault(), style );

	}

	public void setCommunicator( SocketCommunicator communicator ) {
		this.communicator = communicator;
	}

	public long compose() {

		this.lcdCanvas = new Canvas( shell, SWT.EMBEDDED );

		int x = config.getSkinPropertyInt( SkinPropertiesConstants.WINDOW_X, EmulatorConfig.DEFAULT_WINDOW_X );
		int y = config.getSkinPropertyInt( SkinPropertiesConstants.WINDOW_Y, EmulatorConfig.DEFAULT_WINDOW_Y );
		int lcdWidth = config.getArgInt( ArgsConstants.RESOLUTION_WIDTH );
		int lcdHeight = config.getArgInt( ArgsConstants.RESOLUTION_HEIGHT );
		int scale = SkinUtil.getValidScale( config );
//		int rotationId = config.getPropertyShort( PropertiesConstants.WINDOW_ROTATION,
//				EmulatorConfig.DEFAULT_WINDOW_ROTATION );
		// has to be portrait mode at first booting time
		short rotationId = EmulatorConfig.DEFAULT_WINDOW_ROTATION;
		
		composeInternal( lcdCanvas, x, y, lcdWidth, lcdHeight, scale, rotationId, false );

		// sdl uses this handle id.
		windowHandleId = getWindowHandleId();

		return windowHandleId;

	}

	private void composeInternal( Canvas lcdCanvas, int x, int y, int lcdWidth, int lcdHeight, int scale,
			short rotationId, boolean isOnUsbKbd ) {

		lcdCanvas.setBackground( shell.getDisplay().getSystemColor( SWT.COLOR_BLACK ) );

		imageRegistry = ImageRegistry.getInstance();

		shell.setBackground( shell.getDisplay().getSystemColor( SWT.COLOR_BLACK ) );

		shell.setLocation( x, y );

		String emulatorName = SkinUtil.makeEmulatorName( config );
		shell.setText( emulatorName );

		if ( SwtUtil.isWindowsPlatform() ) {
			shell.setImage( imageRegistry.getIcon( IconName.EMULATOR_TITLE_ICO ) );
		} else {
			shell.setImage( imageRegistry.getIcon( IconName.EMULATOR_TITLE ) );
		}

		arrangeSkin( lcdWidth, lcdHeight, scale, rotationId );

		this.isOnUsbKbd = isOnUsbKbd;
		
		if ( null == currentImage ) {
			logger.severe( "Fail to load initial skin image file. Kill this skin process!!!" );
			SkinUtil.openMessage( shell, null, "Fail to load Skin image file.", SWT.ICON_ERROR, config );
			System.exit( -1 );
		}

		seteHoverColor();

		setMenu();

	}

	private void setMenu() {

		Menu contextMenu = new Menu( shell );

		addMenuItems( shell, contextMenu );

		addShellListener( shell );
		addCanvasListener( shell, lcdCanvas );

		shell.setMenu( contextMenu );

	}

//	private void readyToReopen( EmulatorSkin sourceSkin, boolean isOnTop ) {
//
//		logger.info( "Start Changing AlwaysOnTop status" );
//
//		sourceSkin.reopenSkin = new EmulatorSkin( sourceSkin.config, isOnTop );
//
//		sourceSkin.reopenSkin.lcdCanvas = sourceSkin.lcdCanvas;
//		Point previousLocation = sourceSkin.shell.getLocation();
//
//		sourceSkin.reopenSkin.composeInternal( sourceSkin.lcdCanvas, previousLocation.x, previousLocation.y,
//				sourceSkin.currentLcdWidth, sourceSkin.currentLcdHeight, sourceSkin.currentScale,
//				sourceSkin.currentRotationId, sourceSkin.isOnUsbKbd );
//
//		sourceSkin.reopenSkin.windowHandleId = sourceSkin.windowHandleId;
//
//		sourceSkin.reopenSkin.communicator = sourceSkin.communicator;
//		sourceSkin.reopenSkin.communicator.resetSkin( reopenSkin );
//
//		sourceSkin.isAboutToReopen = true;
//		sourceSkin.isShutdownRequested = true;
//
//		if ( sourceSkin.isScreenShotOpened && ( null != sourceSkin.screenShotDialog ) ) {
//			sourceSkin.screenShotDialog.setReserveImage( true );
//			sourceSkin.screenShotDialog.setEmulatorSkin( reopenSkin );
//			reopenSkin.isScreenShotOpened = true;
//			reopenSkin.screenShotDialog = sourceSkin.screenShotDialog;
//			// see open() method to know next logic for screenshot dialog.
//		}
//
//		sourceSkin.lcdCanvas.setParent( reopenSkin.shell );
//		sourceSkin.shell.close();
//
//	}

	private long getWindowHandleId() {

		long windowHandleId = 0;

		if ( SwtUtil.isLinuxPlatform() ) {

			try {
				Field field = lcdCanvas.getClass().getField( "embeddedHandle" );
				windowHandleId = field.getLong( lcdCanvas );
				logger.info( "lcdCanvas.embeddedHandle:" + windowHandleId );
			} catch ( IllegalArgumentException e ) {
				logger.log( Level.SEVERE, e.getMessage(), e );
				shutdown();
			} catch ( IllegalAccessException e ) {
				logger.log( Level.SEVERE, e.getMessage(), e );
				shutdown();
			} catch ( SecurityException e ) {
				logger.log( Level.SEVERE, e.getMessage(), e );
				shutdown();
			} catch ( NoSuchFieldException e ) {
				logger.log( Level.SEVERE, e.getMessage(), e );
				shutdown();
			}

		} else if ( SwtUtil.isWindowsPlatform() ) {

			logger.info( "lcdCanvas.handle:" + lcdCanvas.handle );
			windowHandleId = lcdCanvas.handle;

		} else if ( SwtUtil.isMacPlatform() ) {

			// TODO

		} else {
			logger.severe( "Not Supported OS platform:" + SWT.getPlatform() );
			System.exit( -1 );
		}

		return windowHandleId;

	}

	private void seteHoverColor() {

		ColorsType colors = config.getDbiContents().getColors();
		if ( null != colors ) {
			RgbType hoverRgb = colors.getHoverColor();
			if ( null != hoverRgb ) {
				Long r = hoverRgb.getR();
				Long g = hoverRgb.getG();
				Long b = hoverRgb.getB();
				if ( null != r && null != g && null != b ) {
					hoverColor = new Color( shell.getDisplay(), new RGB( r.intValue(), g.intValue(), b.intValue() ) );
					isDefaultHoverColor = false;
				}
			}
		}

		if ( isDefaultHoverColor ) {
			hoverColor = shell.getDisplay().getSystemColor( SWT.COLOR_WHITE );
		}

	}

	public SkinReopenPolicy open() {

		if ( null == this.communicator ) {
			logger.severe( "communicator is null." );
			return null;
		}

		Display display = this.shell.getDisplay();

		this.shell.open();

		// logic only for reopen case ///////
		if ( isScreenShotOpened && ( null != screenShotDialog ) ) {
			try {
				screenShotDialog.setReserveImage( false );
				screenShotDialog.open();
			} finally {
				isScreenShotOpened = false;
			}
		}
		// ///////////////////////////////////

		while ( !shell.isDisposed() ) {
			if ( !display.readAndDispatch() ) {
				display.sleep();
			}
		}

		return new SkinReopenPolicy( reopenSkin, isAboutToReopen );

	}

	private void arrangeSkin( int lcdWidth, int lcdHeight, int scale, short rotationId ) {

		this.currentLcdWidth = lcdWidth;
		this.currentLcdHeight = lcdHeight;
		this.currentScale = scale;
		this.currentRotationId = rotationId;
		this.currentAngle = SkinRotation.getAngle( rotationId );

		if ( null != currentImage ) {
			currentImage.dispose();
		}
		if ( null != currentKeyPressedImage ) {
			currentKeyPressedImage.dispose();
		}

		shell.redraw();

		currentImage = SkinUtil.createScaledImage( imageRegistry, shell, rotationId, scale, ImageType.IMG_TYPE_MAIN );
		currentKeyPressedImage = SkinUtil.createScaledImage( imageRegistry, shell, rotationId, scale,
				ImageType.IMG_TYPE_PRESSED );

		SkinUtil.trimShell( shell, currentImage );
		SkinUtil.adjustLcdGeometry( lcdCanvas, scale, rotationId );

		if( null != currentImage ) {
			ImageData imageData = currentImage.getImageData();
			shell.setMinimumSize( imageData.width, imageData.height );
			shell.setSize( imageData.width, imageData.height );
		}

	}

	private void addShellListener( final Shell shell ) {

		shellCloseListener = new Listener() {
			@Override
			public void handleEvent( Event event ) {

				if ( isShutdownRequested ) {

					removeShellListeners();
					removeCanvasListeners();

					if ( !isAboutToReopen ) {

						if ( isScreenShotOpened && ( null != screenShotDialog ) ) {
							Shell scShell = screenShotDialog.getShell();
							if ( !scShell.isDisposed() ) {
								scShell.close();
							}
						}

						// save config only for emulator close
						config.setSkinProperty( SkinPropertiesConstants.WINDOW_X, shell.getLocation().x );
						config.setSkinProperty( SkinPropertiesConstants.WINDOW_Y, shell.getLocation().y );
						config.setSkinProperty( SkinPropertiesConstants.WINDOW_SCALE, currentScale );
						config.setSkinProperty( SkinPropertiesConstants.WINDOW_ROTATION, currentRotationId );
						config.setSkinProperty( SkinPropertiesConstants.WINDOW_ONTOP, Boolean.toString( isOnTop ) );
						config.saveSkinProperties();

					}

					if ( null != currentImage ) {
						currentImage.dispose();
					}
					if ( null != currentKeyPressedImage ) {
						currentKeyPressedImage.dispose();
					}

					if ( !isDefaultHoverColor ) {
						hoverColor.dispose();
					}

				} else {

					// Skin have to be alive until receiving shutdown request from qemu.
					event.doit = false;
					if ( null != communicator ) {
						communicator.sendToQEMU( SendCommand.CLOSE, null );
					}

				}

			}
		};

		shell.addListener( SWT.Close, shellCloseListener );

		shellPaintListener = new PaintListener() {

			@Override
			public void paintControl( final PaintEvent e ) {

				// general shell does not support native transparency, so draw image with GC.
				if ( null != currentImage ) {
					e.gc.drawImage( currentImage, 0, 0 );
				}

			}
		};

		shell.addPaintListener( shellPaintListener );

		shellMouseTrackListener = new MouseTrackAdapter() {
			@Override
			public void mouseExit( MouseEvent e ) {
				// MouseMoveListener of shell does not receive event only with MouseMoveListener
				// in case that : hover hardkey -> mouse move into LCD area
				if ( isHoverState ) {
					if ( currentHoverRegion.width == 0 && currentHoverRegion.height == 0 ) {
						shell.redraw();
					} else {
						shell.redraw( currentHoverRegion.x, currentHoverRegion.y, currentHoverRegion.width + 1,
								currentHoverRegion.height + 1, false );
					}
					shell.setToolTipText(null);

					isHoverState = false;
					currentHoverRegion.width = currentHoverRegion.height = 0;
				}
			}

		};

		shell.addMouseTrackListener( shellMouseTrackListener );

		shellMouseMoveListener = new MouseMoveListener() {
			@Override
			public void mouseMove( MouseEvent e ) {
				if ( EmulatorSkin.this.isMousePressed ) {
					if ( 0 == e.button ) { // left button

						SkinRegion hardkeyRegion = SkinUtil.getHardKeyArea( e.x, e.y, currentRotationId, currentScale );

						if ( null == hardkeyRegion ) {
							Point previouseLocation = shell.getLocation();
							int x = previouseLocation.x + ( e.x - EmulatorSkin.this.pressedMouseX );
							int y = previouseLocation.y + ( e.y - EmulatorSkin.this.pressedMouseY );

							shell.setLocation( x, y );
						}

					}
				} else {
					SkinRegion region = SkinUtil.getHardKeyArea( e.x, e.y, currentRotationId, currentScale );

					if ( null == region ) {
						if ( isHoverState ) {
							if ( currentHoverRegion.width == 0 && currentHoverRegion.height == 0 ) {
								shell.redraw();
							} else {
								shell.redraw( currentHoverRegion.x, currentHoverRegion.y, currentHoverRegion.width + 1,
										currentHoverRegion.height + 1, false );
							}
							shell.setToolTipText(null);

							isHoverState = false;
							currentHoverRegion.width = currentHoverRegion.height = 0;
						}
					} else {
						if (isHoverState == false) {
							shell.setToolTipText(SkinUtil.getHardKeyToolTip(e.x, e.y, currentRotationId, currentScale));
						}

						isHoverState = true;
						currentHoverRegion = region;

						/* draw hover */
						shell.getDisplay().asyncExec(new Runnable() {
							public void run() {
								if (currentHoverRegion.width != 0 && currentHoverRegion.height != 0) {
									GC gc = new GC(shell);
									gc.setLineWidth(1);
									gc.setForeground(hoverColor);
									gc.drawRectangle(currentHoverRegion.x, currentHoverRegion.y, currentHoverRegion.width, currentHoverRegion.height);

									gc.dispose();
								}
							}
						});
					}
				}

			} //end of mouseMove
		};

		shell.addMouseMoveListener( shellMouseMoveListener );

		shellMouseListener = new MouseListener() {
			@Override
			public void mouseUp( MouseEvent e ) {
				if ( 1 == e.button ) { // left button
					logger.info( "mouseUp in Skin" );
					EmulatorSkin.this.pressedMouseX = 0;
					EmulatorSkin.this.pressedMouseY = 0;
					EmulatorSkin.this.isMousePressed = false;

					int keyCode = SkinUtil.getHardKeyCode( e.x, e.y, currentRotationId, currentScale );

					if ( SkinUtil.UNKNOWN_KEYCODE != keyCode ) {
						// null check : prevent from mouse up without a hover (ex. doing always on top in hardkey area)
						if ( null != currentHoverRegion ) {
							if ( currentHoverRegion.width == 0 && currentHoverRegion.height == 0 ) {
								shell.redraw();
							} else {
								shell.redraw( currentHoverRegion.x, currentHoverRegion.y, currentHoverRegion.width + 1,
										currentHoverRegion.height + 1, false );
							}
						}

						SkinUtil.trimShell( shell, currentImage );

						KeyEventData keyEventData = new KeyEventData( KeyEventType.RELEASED.value(), keyCode, 0 );
						communicator.sendToQEMU( SendCommand.SEND_HARD_KEY_EVENT, keyEventData );
					}

				}
			}

			@Override
			public void mouseDown( MouseEvent e ) {
				if ( 1 == e.button ) { // left button
					logger.info( "mouseDown in Skin" );
					EmulatorSkin.this.pressedMouseX = e.x;
					EmulatorSkin.this.pressedMouseY = e.y;

					EmulatorSkin.this.isMousePressed = true;

					int keyCode = SkinUtil.getHardKeyCode( e.x, e.y, currentRotationId, currentScale );

					if ( SkinUtil.UNKNOWN_KEYCODE != keyCode ) {
						shell.setToolTipText(null);

						/* draw the button region as the cropped keyPressed image area */
						currentPressedRegion = SkinUtil.getHardKeyArea( e.x, e.y, currentRotationId, currentScale );
						if (currentPressedRegion != null && currentPressedRegion.width != 0 && currentPressedRegion.height != 0) {
							shell.getDisplay().asyncExec(new Runnable() {
								public void run() {
									if ( null != currentKeyPressedImage ) {
										GC gc = new GC( shell );

										/* button */
										gc.drawImage(currentKeyPressedImage,
												currentPressedRegion.x + 1, currentPressedRegion.y + 1,
												currentPressedRegion.width - 1, currentPressedRegion.height - 1, //src
												currentPressedRegion.x + 1, currentPressedRegion.y + 1,
												currentPressedRegion.width - 1, currentPressedRegion.height - 1); //dst

										/* hover */
										if (currentHoverRegion.width != 0 && currentHoverRegion.height != 0) {
											gc.setLineWidth(1);
											gc.setForeground(hoverColor);
											gc.drawRectangle(currentHoverRegion.x, currentHoverRegion.y, currentHoverRegion.width, currentHoverRegion.height);
										}

										gc.dispose();

										SkinUtil.trimShell(shell, currentKeyPressedImage,
												currentPressedRegion.x, currentPressedRegion.y, currentPressedRegion.width, currentPressedRegion.height);

										currentPressedRegion = null;
									}
								}
							});
						}

						KeyEventData keyEventData = new KeyEventData( KeyEventType.PRESSED.value(), keyCode, 0 );
						communicator.sendToQEMU( SendCommand.SEND_HARD_KEY_EVENT, keyEventData );
					}
				}
			}

			@Override
			public void mouseDoubleClick( MouseEvent e ) {
			}
		};

		shell.addMouseListener( shellMouseListener );

	}

	private void removeShellListeners() {

		if ( null != shellCloseListener ) {
			shell.removeListener( SWT.Close, shellCloseListener );
		}
		if ( null != shellPaintListener ) {
			shell.removePaintListener( shellPaintListener );
		}
		if ( null != shellMouseTrackListener ) {
			shell.removeMouseTrackListener( shellMouseTrackListener );
		}
		if ( null != shellMouseMoveListener ) {
			shell.removeMouseMoveListener( shellMouseMoveListener );
		}
		if ( null != shellMouseListener ) {
			shell.removeMouseListener( shellMouseListener );
		}

	}

	private void addCanvasListener( final Shell shell, final Canvas canvas ) {

		canvasMenuDetectListener = new MenuDetectListener() {
			@Override
			public void menuDetected( MenuDetectEvent e ) {
				Menu menu = shell.getMenu();
				lcdCanvas.setMenu( menu );
				menu.setVisible( true );
				e.doit = false;
			}
		};

		// remove 'input method' menu item ( avoid bug )
		canvas.addMenuDetectListener( canvasMenuDetectListener );

		/*canvasDragDetectListener = new DragDetectListener() {

			@Override
			public void dragDetected( DragDetectEvent e ) {
				if ( logger.isLoggable( Level.FINE ) ) {
					logger.fine( "dragDetected e.button:" + e.button );
				}
				if ( 1 == e.button && // left button
						e.x > 0 && e.x < canvas.getSize().x && e.y > 0 && e.y < canvas.getSize().y ) {

					if ( logger.isLoggable( Level.FINE ) ) {
						logger.fine( "dragDetected in LCD" );
					}
					EmulatorSkin.this.isDragStartedInLCD = true;

				}
			}
		};

		canvas.addDragDetectListener( canvasDragDetectListener );*/

		canvasMouseMoveListener = new MouseMoveListener() {

			@Override
			public void mouseMove( MouseEvent e ) {
				if ( true == EmulatorSkin.this.isDragStartedInLCD ) {
					int eventType = MouseEventType.DRAG.value();
					Point canvasSize = canvas.getSize();

					if ( e.x <= 0 ) {
						e.x = 1;
						eventType = MouseEventType.UP.value();
						EmulatorSkin.this.isDragStartedInLCD = false;
					} else if ( e.x >= canvasSize.x ) {
						e.x = canvasSize.x - 1;
						eventType = MouseEventType.UP.value();
						EmulatorSkin.this.isDragStartedInLCD = false;
					}

					if ( e.y <= 0 ) {
						e.y = 1;
						eventType = MouseEventType.UP.value();
						EmulatorSkin.this.isDragStartedInLCD = false;
					} else if ( e.y >= canvasSize.y ) {
						e.y = canvasSize.y - 1;
						eventType = MouseEventType.UP.value();
						EmulatorSkin.this.isDragStartedInLCD = false;
					}

					int[] geometry = SkinUtil.convertMouseGeometry( e.x, e.y, currentLcdWidth, currentLcdHeight,
							currentScale, currentAngle );

					MouseEventData mouseEventData = new MouseEventData( eventType,
							e.x, e.y, geometry[0], geometry[1], 0 );
					communicator.sendToQEMU( SendCommand.SEND_MOUSE_EVENT, mouseEventData );
				}
			}
		};

		canvas.addMouseMoveListener( canvasMouseMoveListener );

		canvasMouseListener = new MouseListener() {

			@Override
			public void mouseUp( MouseEvent e ) {
				if ( 1 == e.button ) { // left button

					int[] geometry = SkinUtil.convertMouseGeometry( e.x, e.y, currentLcdWidth, currentLcdHeight,
							currentScale, currentAngle );

					logger.info( "mouseUp in LCD" + " x:" + geometry[0] + " y:" + geometry[1] );
					MouseEventData mouseEventData = new MouseEventData( MouseEventType.UP.value(),
							e.x, e.y, geometry[0], geometry[1], 0 );
					communicator.sendToQEMU( SendCommand.SEND_MOUSE_EVENT, mouseEventData );
					if ( true == EmulatorSkin.this.isDragStartedInLCD ) {
						EmulatorSkin.this.isDragStartedInLCD = false;
					}
				}
			}

			@Override
			public void mouseDown( MouseEvent e ) {
				if ( 1 == e.button ) { // left button

					int[] geometry = SkinUtil.convertMouseGeometry( e.x, e.y, currentLcdWidth, currentLcdHeight,
							currentScale, currentAngle );

					logger.info( "mouseDown in LCD" + " x:" + geometry[0] + " y:" + geometry[1] );
					MouseEventData mouseEventData = new MouseEventData( MouseEventType.DOWN.value(),
							e.x, e.y, geometry[0], geometry[1], 0 );
					communicator.sendToQEMU( SendCommand.SEND_MOUSE_EVENT, mouseEventData );
					if ( false == EmulatorSkin.this.isDragStartedInLCD ) {
						EmulatorSkin.this.isDragStartedInLCD = true;
					}
				}
			}

			@Override
			public void mouseDoubleClick( MouseEvent e ) {
			}
		};

		canvas.addMouseListener( canvasMouseListener );

		canvasKeyListener = new KeyListener() {
			
			private KeyEvent previous;
			private boolean disappearEvent = false;
			private int disappearKeycode = 0;
			private int disappearKeyLocation = 0;

			@Override
			public void keyReleased( KeyEvent e ) {
				if( logger.isLoggable( Level.INFO ) ) {
					logger.info( "'" + e.character + "':" + e.keyCode + ":" + e.stateMask + ":" + e.keyLocation );
				}else if( logger.isLoggable( Level.FINE ) ) {
					logger.fine( e.toString() );
				}
				int keyCode = e.keyCode | e.stateMask;

				previous = null;
				
				if( SwtUtil.isWindowsPlatform() && disappearEvent) {
					disappearEvent = false;
					if (isMetaKey(e) && e.character != '\0') {
						logger.info( "send previous release : keycode=" + disappearKeycode +
								", disappearKeyLocation=" + disappearKeyLocation);

						KeyEventData keyEventData = new KeyEventData(
								KeyEventType.RELEASED.value(), disappearKeycode, disappearKeyLocation );
						communicator.sendToQEMU( SendCommand.SEND_KEY_EVENT, keyEventData );

						disappearKeycode = 0;
						disappearKeyLocation = 0;
					}
				}
				KeyEventData keyEventData = new KeyEventData( KeyEventType.RELEASED.value(), keyCode, e.keyLocation );
				communicator.sendToQEMU( SendCommand.SEND_KEY_EVENT, keyEventData );
			}

			@Override
			public void keyPressed( KeyEvent e ) {
				int keyCode = e.keyCode | e.stateMask;
				
				if( SwtUtil.isWindowsPlatform() ) {
					if ( null != previous ) {
						if ( previous.keyCode != e.keyCode ) {

							if ( isMetaKey( previous ) ) {
								disappearEvent = true;
								disappearKeycode = keyCode;
								disappearKeyLocation = e.keyLocation;
							} else {
								int previousKeyCode = previous.keyCode | previous.stateMask;
								
								if ( logger.isLoggable( Level.INFO ) ) {
									logger.info( "send previous release : '" + previous.character + "':"
											+ previous.keyCode + ":" + previous.stateMask + ":" + previous.keyLocation );
								} else if ( logger.isLoggable( Level.FINE ) ) {
									logger.fine( "send previous release :" + previous.toString() );
								}
								KeyEventData keyEventData = new KeyEventData( KeyEventType.RELEASED.value(), previousKeyCode,
										previous.keyLocation );
								communicator.sendToQEMU( SendCommand.SEND_KEY_EVENT, keyEventData );
							}

						}
					}
				} //end isWindowsPlatform

				previous = e;

				if ( logger.isLoggable( Level.INFO ) ) {
					logger.info( "'" + e.character + "':" + e.keyCode + ":" + e.stateMask + ":" + e.keyLocation );
				} else if ( logger.isLoggable( Level.FINE ) ) {
					logger.fine( e.toString() );
				}
				KeyEventData keyEventData = new KeyEventData( KeyEventType.PRESSED.value(), keyCode, e.keyLocation );
				communicator.sendToQEMU( SendCommand.SEND_KEY_EVENT, keyEventData );
			}

		};

		canvas.addKeyListener( canvasKeyListener );

	}

	private boolean isMetaKey( KeyEvent event ) {
		if( SWT.CTRL == event.keyCode || SWT.ALT == event.keyCode || SWT.SHIFT == event.keyCode ) {
			return true;
		}
		return false;
	}
	
	private void removeCanvasListeners() {

//		if ( null != canvasDragDetectListener ) {
//			lcdCanvas.removeDragDetectListener( canvasDragDetectListener );
//		}
		if ( null != canvasMouseMoveListener ) {
			lcdCanvas.removeMouseMoveListener( canvasMouseMoveListener );
		}
		if ( null != canvasMouseListener ) {
			lcdCanvas.removeMouseListener( canvasMouseListener );
		}
		if ( null != canvasKeyListener ) {
			lcdCanvas.removeKeyListener( canvasKeyListener );
		}
		if ( null != canvasMenuDetectListener ) {
			lcdCanvas.removeMenuDetectListener( canvasMenuDetectListener );
		}

	}

	private Field getOSField( String field ) {

		String className = "";
		if ( SwtUtil.isLinuxPlatform() ) {
			className = GTK_OS_CLASS;
		} else if ( SwtUtil.isWindowsPlatform() ) {
			className = WIN32_OS_CLASS;
		}

		Field f = null;
		try {
			f = Class.forName( className ).getField( field );
		} catch ( ClassNotFoundException e ) {
			logger.log( Level.SEVERE, e.getMessage(), e );
		} catch ( SecurityException e ) {
			logger.log( Level.SEVERE, e.getMessage(), e );
		} catch ( NoSuchFieldException e ) {
			logger.log( Level.SEVERE, e.getMessage(), e );
		}
		return f;
		
	}

	private Method getOSMethod( String method, Class<?>... parameterTypes ) {

		String className = "";
		if ( SwtUtil.isLinuxPlatform() ) {
			className = GTK_OS_CLASS;
		} else if ( SwtUtil.isWindowsPlatform() ) {
			className = WIN32_OS_CLASS;
		}

		Method m = null;
		try {
			m = Class.forName( className ).getMethod( method, parameterTypes );
		} catch ( ClassNotFoundException e ) {
			logger.log( Level.SEVERE, e.getMessage(), e );
		} catch ( SecurityException e ) {
			logger.log( Level.SEVERE, e.getMessage(), e );
		} catch ( NoSuchMethodException e ) {
			logger.log( Level.SEVERE, e.getMessage(), e );
		}
		return m;

	}

	private Method getOSMethod( String method ) {
		return 	getOSMethod( method, new Class<?>[]{} );
	}

	private Object invokeOSMethod( Method method, Object... args ) {

		if ( null == method ) {
			return null;
		}

		try {
			return method.invoke( null, args );
		} catch ( IllegalArgumentException e ) {
			logger.log( Level.SEVERE, e.getMessage(), e );
		} catch ( IllegalAccessException e ) {
			logger.log( Level.SEVERE, e.getMessage(), e );
		} catch ( InvocationTargetException e ) {
			logger.log( Level.SEVERE, e.getMessage(), e );
		}

		return null;
		
	}

	private Object invokeOSMethod( Method method ) {
		return invokeOSMethod( method, new Object[]{} );
	}

	private boolean setTopMost32(boolean isOnTop) {
		if ( SwtUtil.isLinuxPlatform() ) {
			// reference : http://wmctrl.sourcearchive.com/documentation/1.07/main_8c-source.html

			/* if ( !OS.GDK_WINDOWING_X11() ) {
				logger.warning( "There is no x11 system." );
				return;
			}

			int eventData0 = isOnTop ? 1 : 0; // 'add' or 'remove'

			int topHandle = 0;

			Method m = null;
			try {
				m = Shell.class.getDeclaredMethod( "topHandle", new Class<?>[] {} );
				m.setAccessible( true );
				topHandle = (Integer) m.invoke( shell, new Object[] {} );
			} catch ( SecurityException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
			} catch ( NoSuchMethodException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
			} catch ( IllegalArgumentException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
			} catch ( IllegalAccessException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
			} catch ( InvocationTargetException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
			}

			int xWindow = OS.gdk_x11_drawable_get_xid( OS.GTK_WIDGET_WINDOW( topHandle ) );
			int xDisplay = OS.GDK_DISPLAY();

			byte[] messageBuffer = Converter.wcsToMbcs( null, "_NET_WM_STATE", true );
			int xMessageAtomType = OS.XInternAtom( xDisplay, messageBuffer, false );

			messageBuffer = Converter.wcsToMbcs( null, "_NET_WM_STATE_ABOVE", true );
			int xMessageAtomAbove = OS.XInternAtom( xDisplay, messageBuffer, false );

			XClientMessageEvent event = new XClientMessageEvent();
			event.type = OS.ClientMessage;
			event.window = xWindow;
			event.message_type = xMessageAtomType;
			event.format = 32;
			event.data[0] = eventData0;
			event.data[1] = xMessageAtomAbove;

			int clientEvent = OS.g_malloc( XClientMessageEvent.sizeof );
			OS.memmove( clientEvent, event, XClientMessageEvent.sizeof );
			int rootWin = OS.XDefaultRootWindow( xDisplay );
			// SubstructureRedirectMask:1L<<20 | SubstructureNotifyMask:1L<<19
			OS.XSendEvent( xDisplay, rootWin, false, (int) ( 1L << 20 | 1L << 19 ), clientEvent );
			OS.g_free( clientEvent ); */


			Boolean gdkWindowingX11  = (Boolean) invokeOSMethod( getOSMethod( "GDK_WINDOWING_X11" ) );
			if( null == gdkWindowingX11 ) {
				return false;
			}
			if( !gdkWindowingX11 ) {
				logger.warning( "There is no x11 system." );
				return false;
			}

			int eventData0 = isOnTop ? 1 : 0; // 'add' or 'remove'
			int topHandle = 0;

			Method m = null;
			try {
				m = Shell.class.getDeclaredMethod( "topHandle", new Class<?>[] {} );
				m.setAccessible( true );
				topHandle = (Integer) m.invoke( shell, new Object[] {} );
			} catch ( SecurityException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			} catch ( NoSuchMethodException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			} catch ( IllegalArgumentException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			} catch ( IllegalAccessException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			} catch ( InvocationTargetException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			}

			Integer gtkWidgetWindow = (Integer) invokeOSMethod(
					getOSMethod( "GTK_WIDGET_WINDOW", int.class ), topHandle );
			if( null == gtkWidgetWindow ) {
				return false;
			}

			Integer xWindow = (Integer) invokeOSMethod( getOSMethod( "gdk_x11_drawable_get_xid", int.class ),
					gtkWidgetWindow );
			if( null == xWindow ) {
				return false;
			}

			Integer xDisplay = (Integer) invokeOSMethod( getOSMethod( "GDK_DISPLAY" ) );
			if( null == xDisplay ) {
				return false;
			}

			Method xInternAtom = getOSMethod( "XInternAtom", int.class, byte[].class, boolean.class );

			Class<?> converterClass = null;
			try {
				converterClass = Class.forName( "org.eclipse.swt.internal.Converter" );
			} catch ( ClassNotFoundException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			}

			Method wcsToMbcs = null;
			byte[] messageBufferState = null;
			byte[] messageBufferAbove = null;

			try {
				wcsToMbcs = converterClass.getMethod( "wcsToMbcs", String.class, String.class, boolean.class );
			} catch ( SecurityException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			} catch ( NoSuchMethodException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			}

			try {
				messageBufferState = (byte[]) wcsToMbcs.invoke( null, null, "_NET_WM_STATE", true );
				messageBufferAbove = (byte[]) wcsToMbcs.invoke( null, null, "_NET_WM_STATE_ABOVE", true );
			} catch ( IllegalArgumentException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			} catch ( IllegalAccessException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			} catch ( InvocationTargetException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			}

			Integer xMessageAtomType = (Integer) invokeOSMethod( xInternAtom, xDisplay, messageBufferState, false );
			if( null == xMessageAtomType ) {
				return false;
			}

			Integer xMessageAtomAbove = (Integer) invokeOSMethod( xInternAtom, xDisplay, messageBufferAbove, false );
			if( null == xMessageAtomAbove ) {
				return false;
			}

			Class<?> eventClazz = null;
			Object event = null;
			try {
				eventClazz = Class.forName( "org.eclipse.swt.internal.gtk.XClientMessageEvent" );
				event = eventClazz.newInstance();
			} catch ( InstantiationException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
			} catch ( IllegalAccessException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
			} catch ( ClassNotFoundException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
			}

			if( null == eventClazz || null == event ) {
				return false;
			}

			Integer malloc = null;
			try {

				Field type = eventClazz.getField( "type" );

				Field clientMessageField = getOSField( "ClientMessage" );
				if( null == clientMessageField ) {
					return false;
				}
				type.set( event, clientMessageField.get( null ) );

				Field window = eventClazz.getField( "window" );
				window.set( event, xWindow );
				Field messageType = eventClazz.getField( "message_type" );
				messageType.set( event, xMessageAtomType );
				Field format = eventClazz.getField( "format" );
				format.set( event, 32 );

				Object data = Array.newInstance( int.class, 5 );
				Array.setInt( data, 0, eventData0 );
				Array.setInt( data, 1, xMessageAtomAbove );
				Array.setInt( data, 2, 0 );
				Array.setInt( data, 3, 0 );
				Array.setInt( data, 4, 0 );

				Field dataField = eventClazz.getField( "data" );
				dataField.set( event, data );

				Field sizeofField = eventClazz.getField( "sizeof" );
				Integer sizeof = (Integer) sizeofField.get( null );

				Method gMalloc = getOSMethod( "g_malloc", int.class );
				malloc = (Integer) invokeOSMethod( gMalloc, sizeof );

				Method memmove = getOSMethod( "memmove", int.class, eventClazz, int.class );
				invokeOSMethod( memmove, malloc, event, sizeof );

			} catch ( NoSuchFieldException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			} catch ( IllegalAccessException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			}

			Method xDefaultRootWindow = getOSMethod( "XDefaultRootWindow", int.class );
			Integer rootWin = (Integer) invokeOSMethod( xDefaultRootWindow, xDisplay );

			Method xSendEvent = getOSMethod( "XSendEvent", int.class, int.class, boolean.class,
					int.class, int.class );
			// SubstructureRedirectMask:1L<<20 | SubstructureNotifyMask:1L<<19
			invokeOSMethod( xSendEvent, xDisplay, rootWin, false, (int) ( 1L << 20 | 1L << 19 ), malloc );
			invokeOSMethod( getOSMethod( "g_free", int.class ), malloc ) ;

		} else if( SwtUtil.isWindowsPlatform() ) {
			Point location = shell.getLocation();

			/* int hWndInsertAfter = 0;
			if( isOnTop ) {
				hWndInsertAfter = OS.HWND_TOPMOST;
			} else {
				hWndInsertAfter = OS.HWND_NOTOPMOST;
			}
			OS.SetWindowPos( shell.handle, hWndInsertAfter, location.x, location.y, 0, 0, OS.SWP_NOSIZE ); */

			int hWndInsertAfter = 0;
			int noSize = 0;

			try {
				if ( isOnTop ) {
					Field topMost = getOSField( "HWND_TOPMOST" );
					if ( null == topMost ) {
						return false;
					}
					hWndInsertAfter = topMost.getInt( null );
				} else {
					Field noTopMost = getOSField( "HWND_NOTOPMOST" );
					if ( null == noTopMost ) {
						return false;
					}
					hWndInsertAfter = noTopMost.getInt( null );
				}

				Field noSizeField = getOSField( "SWP_NOSIZE" );
				if ( null == noSizeField ) {
					return false;
				}
				noSize = noSizeField.getInt( null );

			} catch ( IllegalArgumentException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			} catch ( IllegalAccessException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			}

			Method m = getOSMethod( "SetWindowPos", int.class, int.class, int.class, int.class, int.class,
					int.class, int.class );

			invokeOSMethod( m, shell.handle, hWndInsertAfter, location.x, location.y, 0, 0, noSize );
		} else if( SwtUtil.isMacPlatform() ) {
			//TODO:
		}

		return true;
	}

	private boolean setTopMost64(boolean isOnTop)
	{
		if ( SwtUtil.isLinuxPlatform() ) {
			Boolean gdkWindowingX11 = (Boolean) invokeOSMethod( getOSMethod( "GDK_WINDOWING_X11" ) );
			if (null == gdkWindowingX11) {
				return false;
			}
			if (!gdkWindowingX11) {
				logger.warning( "There is no x11 system." );
				return false;
			}

			int eventData0 = isOnTop ? 1 : 0; // 'add' or 'remove'
			long topHandle = 0;

			Method m = null;
			try {
				m = Shell.class.getDeclaredMethod( "topHandle", new Class<?>[] {} );
				m.setAccessible( true );
				topHandle = (Long) m.invoke( shell, new Object[] {} );
			} catch ( SecurityException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			} catch ( NoSuchMethodException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			} catch ( IllegalArgumentException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			} catch ( IllegalAccessException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			} catch ( InvocationTargetException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			}

			Long gtkWidgetWindow = (Long) invokeOSMethod(
					getOSMethod( "GTK_WIDGET_WINDOW", long.class ), topHandle );
			if( null == gtkWidgetWindow ) {
				return false;
			}

			Long xWindow = (Long) invokeOSMethod( getOSMethod( "gdk_x11_drawable_get_xid", long.class ),
					gtkWidgetWindow );
			if( null == xWindow ) {
				return false;
			}

			Long xDisplay = (Long) invokeOSMethod( getOSMethod( "GDK_DISPLAY" ) );
			if( null == xDisplay ) {
				return false;
			}

			Method xInternAtom = getOSMethod( "XInternAtom", long.class, byte[].class, boolean.class );

			Class<?> converterClass = null;
			try {
				converterClass = Class.forName( "org.eclipse.swt.internal.Converter" );
			} catch ( ClassNotFoundException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			}

			Method wcsToMbcs = null;
			byte[] messageBufferState = null;
			byte[] messageBufferAbove = null;

			try {
				wcsToMbcs = converterClass.getMethod( "wcsToMbcs", String.class, String.class, boolean.class );
			} catch ( SecurityException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			} catch ( NoSuchMethodException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			}

			try {
				messageBufferState = (byte[]) wcsToMbcs.invoke( null, null, "_NET_WM_STATE", true );
				messageBufferAbove = (byte[]) wcsToMbcs.invoke( null, null, "_NET_WM_STATE_ABOVE", true );
			} catch ( IllegalArgumentException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			} catch ( IllegalAccessException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			} catch ( InvocationTargetException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			}

			Long xMessageAtomType = (Long) invokeOSMethod( xInternAtom, xDisplay, messageBufferState, false );
			if( null == xMessageAtomType ) {
				return false;
			}

			Long xMessageAtomAbove = (Long) invokeOSMethod( xInternAtom, xDisplay, messageBufferAbove, false );
			if( null == xMessageAtomAbove ) {
				return false;
			}

			Class<?> eventClazz = null;
			Object event = null;
			try {
				eventClazz = Class.forName( "org.eclipse.swt.internal.gtk.XClientMessageEvent" );
				event = eventClazz.newInstance();
			} catch ( InstantiationException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
			} catch ( IllegalAccessException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
			} catch ( ClassNotFoundException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
			}

			if( null == eventClazz || null == event ) {
				return false;
			}

			Long malloc = null;
			try {

				Field type = eventClazz.getField( "type" );

				Field clientMessageField = getOSField( "ClientMessage" );
				if( null == clientMessageField ) {
					return false;
				}
				type.set( event, clientMessageField.get( null ) );

				Field window = eventClazz.getField( "window" );
				window.set( event, xWindow );
				Field messageType = eventClazz.getField( "message_type" );
				messageType.set( event, xMessageAtomType );
				Field format = eventClazz.getField( "format" );
				format.set( event, 32 );

				Object data = Array.newInstance( long.class, 5 );
				Array.setLong( data, 0, eventData0 );
				Array.setLong( data, 1, xMessageAtomAbove );
				Array.setLong( data, 2, 0 );
				Array.setLong( data, 3, 0 );
				Array.setLong( data, 4, 0 );

				Field dataField = eventClazz.getField( "data" );
				dataField.set( event, data );

				Field sizeofField = eventClazz.getField( "sizeof" );
				Integer sizeof = (Integer) sizeofField.get( null );

				Method gMalloc = getOSMethod( "g_malloc", long.class );
				malloc = (Long) invokeOSMethod( gMalloc, sizeof.longValue() );

				Method memmove = getOSMethod( "memmove", long.class, eventClazz, long.class );
				invokeOSMethod( memmove, malloc, event, sizeof.longValue() );

			} catch ( NoSuchFieldException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			} catch ( IllegalAccessException ex ) {
				logger.log( Level.SEVERE, ex.getMessage(), ex );
				return false;
			}

			Method xDefaultRootWindow = getOSMethod( "XDefaultRootWindow", long.class );
			Long rootWin = (Long) invokeOSMethod( xDefaultRootWindow, xDisplay );

			Method xSendEvent = getOSMethod( "XSendEvent", long.class, long.class, boolean.class,
					long.class, long.class );
			// SubstructureRedirectMask:1L<<20 | SubstructureNotifyMask:1L<<19
			invokeOSMethod( xSendEvent, xDisplay, rootWin, false, (long) ( 1L << 20 | 1L << 19 ), malloc );
			invokeOSMethod( getOSMethod( "g_free", long.class ), malloc );
		} else if (SwtUtil.isWindowsPlatform()) {
			//TODO:
		} else if( SwtUtil.isMacPlatform() ) {
			//TODO:
		}

		return true;
	}

	private void addMenuItems( final Shell shell, final Menu menu ) {

		final MenuItem detailInfoItem = new MenuItem( menu, SWT.PUSH );

		String emulatorName = SkinUtil.makeEmulatorName( config );
		detailInfoItem.setText( emulatorName );
		detailInfoItem.setImage( imageRegistry.getIcon( IconName.DETAIL_INFO ) );
		detailInfoItem.addSelectionListener( new SelectionAdapter() {
			@Override
			public void widgetSelected( SelectionEvent e ) {
				if ( logger.isLoggable( Level.FINE ) ) {
					logger.fine( "Open detail info" );
				}
				String emulatorName = SkinUtil.makeEmulatorName( config );
				DetailInfoDialog detailInfoDialog = new DetailInfoDialog( shell, emulatorName, communicator, config );
				detailInfoDialog.open();
			}
		} );

		new MenuItem( menu, SWT.SEPARATOR );

		final MenuItem onTopItem = new MenuItem( menu, SWT.CHECK );
		onTopItem.setText( "&Always On Top" );
		onTopItem.setSelection( isOnTop );

		onTopItem.addSelectionListener( new SelectionAdapter() {
			@Override
			public void widgetSelected( SelectionEvent e ) {

				final boolean isOnTop = onTopItem.getSelection();

				if ( logger.isLoggable( Level.FINE ) ) {
					logger.fine( "Select Always On Top : " + isOnTop );
				}

				// readyToReopen( EmulatorSkin.this, isOnTop );

				//internal/Library.java::arch()
				String osArch = System.getProperty("os.arch"); //$NON-NLS-1$
				logger.info(osArch);
				if (osArch.equals("amd64") || osArch.equals("IA64N") || osArch.equals("IA64W")) { //$NON-NLS-1$ $NON-NLS-2$ $NON-NLS-3$
					logger.info("64bit architecture");
					setTopMost64(isOnTop); //64bit
				} else {
					logger.info("32bit architecture");
					setTopMost32(isOnTop);
				}
			}
		} );

		final MenuItem rotateItem = new MenuItem( menu, SWT.CASCADE );
		rotateItem.setText( "&Rotate" );
		rotateItem.setImage( imageRegistry.getIcon( IconName.ROTATE ) );
		Menu rotateMenu = createRotateMenu( menu.getShell() );
		rotateItem.setMenu( rotateMenu );

		final MenuItem scaleItem = new MenuItem( menu, SWT.CASCADE );
		scaleItem.setText( "&Scale" );
		scaleItem.setImage( imageRegistry.getIcon( IconName.SCALE ) );
		Menu scaleMenu = createScaleMenu( menu.getShell() );
		scaleItem.setMenu( scaleMenu );

		new MenuItem( menu, SWT.SEPARATOR );

		final MenuItem advancedItem = new MenuItem( menu, SWT.CASCADE );
		advancedItem.setText( "Ad&vanced" );
		advancedItem.setImage( imageRegistry.getIcon( IconName.ADVANCED ) );
		Menu advancedMenu = createAdvancedMenu( menu.getShell() );
		advancedItem.setMenu( advancedMenu );

		final MenuItem shellItem = new MenuItem( menu, SWT.PUSH );
		shellItem.setText( "S&hell" );
		shellItem.setImage( imageRegistry.getIcon( IconName.SHELL ) );
		
		shellItem.addSelectionListener( new SelectionAdapter() {
			@Override
			public void widgetSelected( SelectionEvent e ) {
				if ( !communicator.isSensorDaemonStarted() ) {
					SkinUtil.openMessage( shell, null, "SDB is not ready.\nPlease, wait.", SWT.ICON_WARNING, config );
					return;
				}

				String sdbPath = SkinUtil.getSdbPath();

				File sdbFile = new File(sdbPath);
				if (!sdbFile.exists()) {
					logger.log( Level.INFO, "SDB file is not exist : " + sdbFile.getAbsolutePath());
					try {
						SkinUtil.openMessage( shell, null,
								"SDB file is not exist in the following path.\n" + sdbFile.getCanonicalPath()
								, SWT.ICON_ERROR, config );
					} catch (IOException ee) {
						logger.log( Level.SEVERE, ee.getMessage(), ee );
					}
					return;
				}

				int portSdb = config.getArgInt( ArgsConstants.NET_BASE_PORT );

				ProcessBuilder procSdb = new ProcessBuilder();

				if ( SwtUtil.isLinuxPlatform() ) {
					procSdb.command( "/usr/bin/gnome-terminal", "--disable-factory",
							"--title=" + SkinUtil.makeEmulatorName( config ), "-x", sdbPath, "-s", "emulator-"
									+ portSdb, "shell" );
				} else if ( SwtUtil.isWindowsPlatform() ) {
					procSdb.command( "cmd.exe", "/c", "start", sdbPath, "-s", "emulator-" + portSdb, "shell" );
				}
				logger.log( Level.INFO, procSdb.command().toString() );

				try {
					procSdb.start(); // open sdb shell
				} catch ( Exception ee ) {
					logger.log( Level.SEVERE, ee.getMessage(), ee );
					SkinUtil.openMessage( shell, null, "Fail to open Shell.", SWT.ICON_ERROR, config );
				}

				communicator.sendToQEMU( SendCommand.OPEN_SHELL, null );
			}
		} );

		new MenuItem( menu, SWT.SEPARATOR );

		MenuItem closeItem = new MenuItem( menu, SWT.PUSH );
		closeItem.setText( "&Close" );
		closeItem.setImage( imageRegistry.getIcon( IconName.CLOSE ) );
		closeItem.addSelectionListener( new SelectionAdapter() {
			@Override
			public void widgetSelected( SelectionEvent e ) {
				communicator.sendToQEMU( SendCommand.CLOSE, null );
			}
		} );

	}

	private Menu createRotateMenu( final Shell shell ) {

		Menu menu = new Menu( shell, SWT.DROP_DOWN );

		final List<MenuItem> rotationList = new ArrayList<MenuItem>();

		Iterator<Entry<Short, RotationType>> iterator = SkinRotation.getRotationIterator();

		while ( iterator.hasNext() ) {

			Entry<Short, RotationType> entry = iterator.next();
			Short rotationId = entry.getKey();
			RotationType section = entry.getValue();

			final MenuItem menuItem = new MenuItem( menu, SWT.RADIO );
			menuItem.setText( section.getName().value() );
			menuItem.setData( rotationId );

			if ( currentRotationId == rotationId ) {
				menuItem.setSelection( true );
			}

			rotationList.add( menuItem );

		}

		SelectionAdapter selectionAdapter = new SelectionAdapter() {
			@Override
			public void widgetSelected( SelectionEvent e ) {

				MenuItem item = (MenuItem) e.getSource();

				boolean selection = item.getSelection();

				if ( !selection ) {
					return;
				}

				if ( !communicator.isSensorDaemonStarted() ) {

					// reset selection.
					item.setSelection( false );

					for ( MenuItem m : rotationList ) {
						short rotationId = (Short) m.getData();
						if ( rotationId == currentRotationId ) {
							m.setSelection( true );
							break;
						}
					}
					// /////////

					SkinUtil.openMessage( shell, null, "Rotation is not ready.\nPlease, wait.", SWT.ICON_WARNING,
							config );

					return;

				}

				short rotationId = ( (Short) item.getData() );

				arrangeSkin( currentLcdWidth, currentLcdHeight, currentScale, rotationId );
				LcdStateData lcdStateData = new LcdStateData( currentScale, rotationId );
				communicator.sendToQEMU( SendCommand.CHANGE_LCD_STATE, lcdStateData );
			}
		};

		for ( MenuItem menuItem : rotationList ) {
			menuItem.addSelectionListener( selectionAdapter );
		}

		return menu;
	}

	private Menu createScaleMenu( Shell shell ) {

		Menu menu = new Menu( shell, SWT.DROP_DOWN );

		final List<MenuItem> scaleList = new ArrayList<MenuItem>();

		final MenuItem scaleOneItem = new MenuItem( menu, SWT.RADIO );
		scaleOneItem.setText( "1x" );
		scaleOneItem.setData( Scale.SCALE_100 );
		scaleList.add( scaleOneItem );

		final MenuItem scaleThreeQtrItem = new MenuItem( menu, SWT.RADIO );
		scaleThreeQtrItem.setText( "3/4x" );
		scaleThreeQtrItem.setData( Scale.SCALE_75 );
		scaleList.add( scaleThreeQtrItem );

		final MenuItem scalehalfItem = new MenuItem( menu, SWT.RADIO );
		scalehalfItem.setText( "1/2x" );
		scalehalfItem.setData( Scale.SCALE_50 );
		scaleList.add( scalehalfItem );

		final MenuItem scaleOneQtrItem = new MenuItem( menu, SWT.RADIO );
		scaleOneQtrItem.setText( "1/4x" );
		scaleOneQtrItem.setData( Scale.SCALE_25 );
		scaleList.add( scaleOneQtrItem );

		SelectionAdapter selectionAdapter = new SelectionAdapter() {

			@Override
			public void widgetSelected( SelectionEvent e ) {

				MenuItem item = (MenuItem) e.getSource();

				boolean selection = item.getSelection();

				if ( !selection ) {
					return;
				}

				int scale = ( (Scale) item.getData() ).value();

				arrangeSkin( currentLcdWidth, currentLcdHeight, scale, currentRotationId );
				LcdStateData lcdStateData = new LcdStateData( scale, currentRotationId );
				communicator.sendToQEMU( SendCommand.CHANGE_LCD_STATE, lcdStateData );

			}
		};
		
		for ( MenuItem menuItem : scaleList ) {

			int scale = ( (Scale) menuItem.getData() ).value();
			if ( scale == currentScale ) {
				menuItem.setSelection( true );
			}

			menuItem.addSelectionListener( selectionAdapter );

		}

		return menu;

	}

	private Menu createAdvancedMenu( final Shell shell ) {

		final Menu menu = new Menu( shell, SWT.DROP_DOWN );

		final MenuItem screenshotItem = new MenuItem( menu, SWT.PUSH );
		screenshotItem.setText( "&Screen Shot" );
		screenshotItem.setImage( imageRegistry.getIcon( IconName.SCREENSHOT ) );
		screenshotItem.addSelectionListener( new SelectionAdapter() {

			@Override
			public void widgetSelected( SelectionEvent e ) {

				if ( isScreenShotOpened ) {
					return;
				}

				try {

					isScreenShotOpened = true;

					screenShotDialog = new ScreenShotDialog( shell, communicator, EmulatorSkin.this, config,
							imageRegistry.getIcon(IconName.SCREENSHOT) );
					screenShotDialog.open();

				} catch ( ScreenShotException ex ) {

					logger.log( Level.SEVERE, ex.getMessage(), ex );
					SkinUtil.openMessage( shell, null, "Fail to create a screen shot.", SWT.ICON_ERROR, config );

				} catch ( Exception ex ) {

					// defense exception handling.
					logger.log( Level.SEVERE, ex.getMessage(), ex );
					String errorMessage = "Internal Error.\n[" + ex.getMessage() + "]";
					SkinUtil.openMessage( shell, null, errorMessage, SWT.ICON_ERROR, config );

				} finally {
					isScreenShotOpened = false;
				}

			}
		} );

		final MenuItem usbKeyboardItem = new MenuItem( menu, SWT.CASCADE );
		usbKeyboardItem.setText( "&USB Keyboard" );
		usbKeyboardItem.setImage( imageRegistry.getIcon( IconName.USB_KEBOARD ) );
		
		Menu usbKeyBoardMenu = new Menu( shell, SWT.DROP_DOWN );

		final MenuItem usbOnItem = new MenuItem( usbKeyBoardMenu, SWT.RADIO );
		usbOnItem.setText( "On" );
		usbOnItem.setSelection( isOnUsbKbd );
		
		final MenuItem usbOffItem = new MenuItem( usbKeyBoardMenu, SWT.RADIO );
		usbOffItem.setText( "Off" );
		usbOffItem.setSelection( !isOnUsbKbd );

		SelectionAdapter usbSelectionAdaptor = new SelectionAdapter() {
			@Override
			public void widgetSelected( SelectionEvent e ) {
				MenuItem item = (MenuItem) e.getSource();
				if ( item.getSelection() ) {
					boolean on = item.equals( usbOnItem );
					isOnUsbKbd = on;
					communicator
							.sendToQEMU( SendCommand.USB_KBD, new BooleanData( on, SendCommand.USB_KBD.toString() ) );
				}

			}
		};

		usbOnItem.addSelectionListener( usbSelectionAdaptor );
		usbOffItem.addSelectionListener( usbSelectionAdaptor );

		usbKeyboardItem.setMenu( usbKeyBoardMenu );

		final MenuItem aboutItem = new MenuItem( menu, SWT.PUSH );
		aboutItem.setText( "&About" );
		aboutItem.setImage( imageRegistry.getIcon( IconName.ABOUT ) );

		aboutItem.addSelectionListener( new SelectionAdapter() {
			private boolean isOpen;

			@Override
			public void widgetSelected( SelectionEvent e ) {
				if ( !isOpen ) {
					isOpen = true;
					AboutDialog dialog = new AboutDialog( shell );
					dialog.open();
					isOpen = false;
				}
			}
		} );

		return menu;

	}

	public void shutdown() {

		isShutdownRequested = true;

		if ( !this.shell.isDisposed() ) {
			this.shell.getDisplay().asyncExec( new Runnable() {
				@Override
				public void run() {
					if ( !shell.isDisposed() ) {
						EmulatorSkin.this.shell.close();
					}
				}
			} );
		}

	}

	public short getCurrentRotationId() {
		return currentRotationId;
	}

}
