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
import java.util.LinkedList;
import java.util.List;
import java.util.Map.Entry;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.FocusEvent;
import org.eclipse.swt.events.FocusListener;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.KeyListener;
import org.eclipse.swt.events.MenuDetectEvent;
import org.eclipse.swt.events.MenuDetectListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.MouseWheelListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Color;
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
import org.tizen.emulator.skin.comm.ICommunicator.MouseButtonType;
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
import org.tizen.emulator.skin.dialog.RamdumpDialog;
import org.tizen.emulator.skin.image.ImageRegistry;
import org.tizen.emulator.skin.image.ImageRegistry.IconName;
import org.tizen.emulator.skin.info.SkinInformation;
import org.tizen.emulator.skin.layout.GeneralPurposeSkinComposer;
import org.tizen.emulator.skin.layout.ISkinComposer;
import org.tizen.emulator.skin.layout.PhoneShapeSkinComposer;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.screenshot.ScreenShotDialog;
import org.tizen.emulator.skin.util.SkinRotation;
import org.tizen.emulator.skin.util.SkinUtil;
import org.tizen.emulator.skin.util.SwtUtil;
import org.tizen.emulator.skin.window.SkinWindow;

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
	public static final String COCOA_OS_CLASS = "org.eclipse.swt.internal.cocoa.OS";

	private Logger logger = SkinLogger.getSkinLogger( EmulatorSkin.class ).getLogger();

	protected EmulatorConfig config;
	protected Shell shell;
	protected ImageRegistry imageRegistry;
	protected Canvas lcdCanvas;
	protected SkinInformation skinInfo;
	protected ISkinComposer skinComposer;

	protected EmulatorSkinState currentState;

	private boolean isDragStartedInLCD;
	private boolean isShutdownRequested;
	private boolean isAboutToReopen;
	private boolean isOnTop;
	private boolean isOnKbd;

	private SkinWindow controlPanel; //not used yet
	protected ScreenShotDialog screenShotDialog;
	private Menu contextMenu;

	protected SocketCommunicator communicator;

	private Listener shellCloseListener;
	private MenuDetectListener shellMenuDetectListener;

	//private DragDetectListener canvasDragDetectListener;
	private MouseMoveListener canvasMouseMoveListener;
	private MouseListener canvasMouseListener;
	private MouseWheelListener canvasMouseWheelListener;
	private KeyListener canvasKeyListener;
	private MenuDetectListener canvasMenuDetectListener;
	private FocusListener canvasFocusListener;

	private LinkedList<KeyEventData> pressedKeyEventList;

	private EmulatorSkin reopenSkin;
	
	/**
	 * @brief constructor
	 * @param config : configuration of emulator skin
	 * @param isOnTop : always on top flag
	*/
	protected EmulatorSkin(EmulatorConfig config, SkinInformation skinInfo, boolean isOnTop) {
		this.config = config;
		this.skinInfo = skinInfo;
		this.isOnTop = isOnTop;
		this.pressedKeyEventList = new LinkedList<KeyEventData>();

		int style = SWT.NO_TRIM | SWT.DOUBLE_BUFFERED;
		if (skinInfo.isPhoneShape() == false) {
			style = SWT.TITLE | SWT.CLOSE | SWT.MIN | SWT.BORDER;
		}
		this.shell = new Shell(Display.getDefault(), style);

		this.currentState = new EmulatorSkinState();
	}

	public void setCommunicator( SocketCommunicator communicator ) {
		this.communicator = communicator;
	}

	public long initLayout() {
		imageRegistry = ImageRegistry.getInstance();

		if (skinInfo.isPhoneShape() == true) { /* phone shape skin */
			skinComposer = new PhoneShapeSkinComposer(config, shell,
					currentState, imageRegistry, communicator);

			((PhoneShapeSkinComposer) skinComposer).addPhoneShapeListener(shell);
		} else { /* general purpose skin */
			skinComposer = new GeneralPurposeSkinComposer(config, shell,
					currentState, imageRegistry, communicator);
		}

		lcdCanvas = skinComposer.compose();

		/* load a hover color */
		currentState.setHoverColor(loadHoverColor());

		/* added event handlers */
		addShellListener(shell);
		addCanvasListener(shell, lcdCanvas);

		/* attach a menu */
		this.isOnKbd = false;
		setMenu();

		return 0;
	}

	private void setMenu() {
		contextMenu = new Menu(shell);

		addMenuItems(shell, contextMenu);

		shell.setMenu(contextMenu);
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
//				sourceSkin.currentRotationId, sourceSkin.isOnKbd );
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

	private Color loadHoverColor() {
		ColorsType colors = config.getDbiContents().getColors();

		if (null != colors) {
			RgbType hoverRgb = colors.getHoverColor();
			if (null != hoverRgb) {
				Long r = hoverRgb.getR();
				Long g = hoverRgb.getG();
				Long b = hoverRgb.getB();
				if (null != r && null != g && null != b) {
					Color hoverColor = new Color(shell.getDisplay(),
							new RGB(r.intValue(), g.intValue(), b.intValue()));
					return hoverColor;
				}
			}
		}

		/* white */
		return (new Color(shell.getDisplay(), new RGB(255, 255, 255)));
	}

	public SkinReopenPolicy open() {

		if ( null == this.communicator ) {
			logger.severe( "communicator is null." );
			return null;
		}

		Display display = this.shell.getDisplay();

		this.shell.open();

		// logic only for reopen case ///////
//		if ( isScreenShotOpened && ( null != screenShotDialog ) ) {
//			try {
//				screenShotDialog.setReserveImage( false );
//				screenShotDialog.open();
//			} finally {
//				isScreenShotOpened = false;
//			}
//		}
		// ///////////////////////////////////

		while ( !shell.isDisposed() ) {
			if ( !display.readAndDispatch() ) {
				display.sleep();
			}
		}

		return new SkinReopenPolicy( reopenSkin, isAboutToReopen );

	}

	protected void skinFinalize() {
		skinComposer.composerFinalize();
	}

	private void addShellListener(final Shell shell) {

		shellCloseListener = new Listener() {
			@Override
			public void handleEvent( Event event ) {

				if ( isShutdownRequested ) {

					removeShellListeners();
					removeCanvasListeners();

					if ( !isAboutToReopen ) {
						/* close the screen shot window */
						if (null != screenShotDialog) {
							Shell scShell = screenShotDialog.getShell();
							if (!scShell.isDisposed()) {
								scShell.close();
							}
							screenShotDialog = null;
						}

						/* close the control panel window */
						if (null != controlPanel) {
							Shell cpShell = controlPanel.getShell();
							if (!cpShell.isDisposed()) {
								cpShell.close();
							}
							controlPanel = null;
						}

						/* save config only for emulator close */
						config.setSkinProperty(
								SkinPropertiesConstants.WINDOW_X, shell.getLocation().x);
						config.setSkinProperty(
								SkinPropertiesConstants.WINDOW_Y, shell.getLocation().y);
						config.setSkinProperty(
								SkinPropertiesConstants.WINDOW_SCALE, currentState.getCurrentScale());
						config.setSkinProperty(
								SkinPropertiesConstants.WINDOW_ROTATION, currentState.getCurrentRotationId());
						config.setSkinProperty(
								SkinPropertiesConstants.WINDOW_ONTOP, Boolean.toString(isOnTop));
						config.saveSkinProperties();
					}

					if (currentState.getCurrentImage() != null) {
						currentState.getCurrentImage().dispose();
					}
					if (currentState.getCurrentKeyPressedImage() != null) {
						currentState.getCurrentKeyPressedImage().dispose();
					}

					if (currentState.getHoverColor() != null) {
						currentState.getHoverColor().dispose();
					}

					skinFinalize();

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

		shellMenuDetectListener = new MenuDetectListener() {
			@Override
			public void menuDetected(MenuDetectEvent e) {
				Menu menu = EmulatorSkin.this.contextMenu;

				if (menu != null) {
					shell.setMenu(menu);
					menu.setVisible(true);
					e.doit = false;
				} else {
					shell.setMenu(null);
				}
			}
		};

		shell.addMenuDetectListener(shellMenuDetectListener);
	}

	private void removeShellListeners() {
		if (null != shellCloseListener) {
			shell.removeListener(SWT.Close, shellCloseListener);
		}

		if (null != shellMenuDetectListener) {
			shell.removeMenuDetectListener(shellMenuDetectListener);
		}
	}

	private void addCanvasListener(final Shell shell, final Canvas canvas) {
		/* menu */
		canvasMenuDetectListener = new MenuDetectListener() {
			@Override
			public void menuDetected(MenuDetectEvent e) {
				Menu menu = EmulatorSkin.this.contextMenu;

				if (menu != null && EmulatorSkin.this.isDragStartedInLCD == false) {
					lcdCanvas.setMenu(menu);
					menu.setVisible(true);
					e.doit = false;
				} else {
					lcdCanvas.setMenu(null);
				}
			}
		};

		// remove 'input method' menu item ( avoid bug )
		canvas.addMenuDetectListener(canvasMenuDetectListener);

		/* focus */
		canvasFocusListener = new FocusListener() {
			@Override
			public void focusGained(FocusEvent event) {
				logger.info("gain focus");
			}

			public void focusLost(FocusEvent event) {
				logger.info("lost focus");

				/* key event compensation */
				if (pressedKeyEventList.isEmpty() == false) {
					for (KeyEventData data : pressedKeyEventList) {
						KeyEventData keyEventData = new KeyEventData(
								KeyEventType.RELEASED.value(), data.keycode,
								data.stateMask, data.keyLocation);
						communicator.sendToQEMU(SendCommand.SEND_KEY_EVENT, keyEventData);

						logger.info("auto release : keycode=" + keyEventData.keycode +
								", stateMask=" + keyEventData.stateMask +
								", keyLocation=" + keyEventData.keyLocation);
					}
				}

				pressedKeyEventList.clear();
			}
		};

		lcdCanvas.addFocusListener(canvasFocusListener);

		/* mouse event */
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

					if ( e.x < 0 ) {
						e.x = 0;
						eventType = MouseEventType.RELEASE.value();
						EmulatorSkin.this.isDragStartedInLCD = false;
					} else if ( e.x >= canvasSize.x ) {
						e.x = canvasSize.x - 1;
						eventType = MouseEventType.RELEASE.value();
						EmulatorSkin.this.isDragStartedInLCD = false;
					}

					if ( e.y < 0 ) {
						e.y = 0;
						eventType = MouseEventType.RELEASE.value();
						EmulatorSkin.this.isDragStartedInLCD = false;
					} else if ( e.y >= canvasSize.y ) {
						e.y = canvasSize.y - 1;
						eventType = MouseEventType.RELEASE.value();
						EmulatorSkin.this.isDragStartedInLCD = false;
					}

					int[] geometry = SkinUtil.convertMouseGeometry(e.x, e.y,
							currentState.getCurrentResolutionWidth(),
							currentState.getCurrentResolutionHeight(),
							currentState.getCurrentScale(), currentState.getCurrentAngle());

					MouseEventData mouseEventData = new MouseEventData(
							MouseButtonType.LEFT.value(), eventType,
							e.x, e.y, geometry[0], geometry[1], 0);

					communicator.sendToQEMU( SendCommand.SEND_MOUSE_EVENT, mouseEventData );
				}
			}
		};

		canvas.addMouseMoveListener( canvasMouseMoveListener );

		canvasMouseListener = new MouseListener() {

			@Override
			public void mouseUp( MouseEvent e ) {
				if ( 1 == e.button ) { // left button

					int[] geometry = SkinUtil.convertMouseGeometry(e.x, e.y,
							currentState.getCurrentResolutionWidth(),
							currentState.getCurrentResolutionHeight(),
							currentState.getCurrentScale(), currentState.getCurrentAngle());
					logger.info( "mouseUp in LCD" + " x:" + geometry[0] + " y:" + geometry[1] );

					MouseEventData mouseEventData = new MouseEventData(
							MouseButtonType.LEFT.value(), MouseEventType.RELEASE.value(),
							e.x, e.y, geometry[0], geometry[1], 0);

					communicator.sendToQEMU( SendCommand.SEND_MOUSE_EVENT, mouseEventData );
					if ( true == EmulatorSkin.this.isDragStartedInLCD ) {
						EmulatorSkin.this.isDragStartedInLCD = false;
					}
				} else if (2 == e.button) { // wheel button
					logger.info("wheelUp in LCD");
				}
			}

			@Override
			public void mouseDown( MouseEvent e ) {
				if ( 1 == e.button ) { // left button

					int[] geometry = SkinUtil.convertMouseGeometry(e.x, e.y,
							currentState.getCurrentResolutionWidth(),
							currentState.getCurrentResolutionHeight(),
							currentState.getCurrentScale(), currentState.getCurrentAngle());
					logger.info( "mouseDown in LCD" + " x:" + geometry[0] + " y:" + geometry[1] );

					MouseEventData mouseEventData = new MouseEventData(
							MouseButtonType.LEFT.value(), MouseEventType.PRESS.value(),
							e.x, e.y, geometry[0], geometry[1], 0);

					communicator.sendToQEMU( SendCommand.SEND_MOUSE_EVENT, mouseEventData );
					if ( false == EmulatorSkin.this.isDragStartedInLCD ) {
						EmulatorSkin.this.isDragStartedInLCD = true;
					}
				}
			}

			@Override
			public void mouseDoubleClick( MouseEvent e ) {
				/* do nothing */
			}
		};

		canvas.addMouseListener( canvasMouseListener );

		canvasMouseWheelListener = new MouseWheelListener() {

			@Override
			public void mouseScrolled(MouseEvent e) {
				int[] geometry = SkinUtil.convertMouseGeometry(e.x, e.y,
						currentState.getCurrentResolutionWidth(),
						currentState.getCurrentResolutionHeight(),
						currentState.getCurrentScale(), currentState.getCurrentAngle());
				logger.info("mousewheel in LCD" +
						" x:" + geometry[0] + " y:" + geometry[1] + " value:" + e.count);

				int eventType;
				if (e.count < 0) {
					eventType = MouseEventType.WHEELDOWN.value();
				} else {
					eventType = MouseEventType.WHEELUP.value();
				}

				MouseEventData mouseEventData = new MouseEventData(
						MouseButtonType.WHEEL.value(), eventType,
						e.x, e.y, geometry[0], geometry[1], e.count);
				communicator.sendToQEMU(SendCommand.SEND_MOUSE_EVENT, mouseEventData);
			}
		};

		canvas.addMouseWheelListener( canvasMouseWheelListener );

		/* keyboard event */
		canvasKeyListener = new KeyListener() {

			private KeyEvent previous;
			private boolean disappearEvent = false;
			private int disappearKeycode = 0;
			private int disappearStateMask = 0;
			private int disappearKeyLocation = 0;

			@Override
			public void keyReleased( KeyEvent e ) {
				if ( logger.isLoggable( Level.INFO ) ) {
					logger.info( "'" + e.character + "':" + e.keyCode + ":" + e.stateMask + ":" + e.keyLocation );
				} else if ( logger.isLoggable( Level.FINE ) ) {
					logger.fine( e.toString() );
				}
				int keyCode = e.keyCode;
				int stateMask = e.stateMask;

				previous = null;

				/* generate a disappeared key event in Windows */
				if (SwtUtil.isWindowsPlatform() && disappearEvent) {
					disappearEvent = false;
					if (isMetaKey(e) && e.character != '\0') {
						logger.info("send previous release : keycode=" + disappearKeycode +
								", stateMask=" + disappearStateMask +
								", keyLocation=" + disappearKeyLocation);

						KeyEventData keyEventData = new KeyEventData(
								KeyEventType.RELEASED.value(),
								disappearKeycode, disappearStateMask, disappearKeyLocation);
						communicator.sendToQEMU(SendCommand.SEND_KEY_EVENT, keyEventData);

						removePressedKey(keyEventData);

						disappearKeycode = 0;
						disappearStateMask = 0;
						disappearKeyLocation = 0;
					}
				}

				KeyEventData keyEventData = new KeyEventData(
						KeyEventType.RELEASED.value(), keyCode, stateMask, e.keyLocation);
				communicator.sendToQEMU(SendCommand.SEND_KEY_EVENT, keyEventData);

				removePressedKey(keyEventData);
			}

			@Override
			public void keyPressed( KeyEvent e ) {
				int keyCode = e.keyCode;
				int stateMask = e.stateMask;

				/* When the pressed key event is overwritten by next key event,
				 * the release event of previous key does not occur in Windows.
				 * So, we generate a release key event by ourselves
				 * that had been disappeared. */
				if (SwtUtil.isWindowsPlatform()) {
					if (null != previous) {
						if (previous.keyCode != e.keyCode) {

							if (isMetaKey(previous)) {
								disappearEvent = true;
								disappearKeycode = keyCode;
								disappearStateMask = stateMask;
								disappearKeyLocation = e.keyLocation;
							} else {
								int previousKeyCode = previous.keyCode;
								int previousStateMask = previous.stateMask;

								if ( logger.isLoggable( Level.INFO ) ) {
									logger.info( "send previous release : '" + previous.character + "':"
											+ previous.keyCode + ":" + previous.stateMask + ":" + previous.keyLocation );
								} else if ( logger.isLoggable( Level.FINE ) ) {
									logger.fine( "send previous release :" + previous.toString() );
								}

								KeyEventData keyEventData = new KeyEventData(KeyEventType.RELEASED.value(),
										previousKeyCode, previousStateMask, previous.keyLocation);
								communicator.sendToQEMU(SendCommand.SEND_KEY_EVENT, keyEventData);

								removePressedKey(keyEventData);
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

				KeyEventData keyEventData = new KeyEventData(
						KeyEventType.PRESSED.value(), keyCode, stateMask, e.keyLocation);
				communicator.sendToQEMU(SendCommand.SEND_KEY_EVENT, keyEventData);

				addPressedKey(keyEventData);
			}

		};

		canvas.addKeyListener( canvasKeyListener );

	}

	private boolean isMetaKey(KeyEvent event) {
		if (SWT.CTRL == event.keyCode ||
				SWT.ALT == event.keyCode ||
				SWT.SHIFT == event.keyCode) {
			return true;
		}
		return false;
	}

	private synchronized boolean addPressedKey(KeyEventData pressData) {
		for (KeyEventData data : pressedKeyEventList) {
			if (data.keycode == pressData.keycode &&
					data.stateMask == pressData.stateMask &&
					data.keyLocation == pressData.keyLocation) {
				return false;
			}
		}

		pressedKeyEventList.add(pressData);
		return true;
	}

	private synchronized boolean removePressedKey(KeyEventData releaseData) {

		for (KeyEventData data : pressedKeyEventList) {
			if (data.keycode == releaseData.keycode &&
					data.stateMask == releaseData.stateMask &&
					data.keyLocation == releaseData.keyLocation) {
				pressedKeyEventList.remove(data);

				return true;
			}
		}

		return false;
	}

	private void removeCanvasListeners() {
//		if ( null != canvasDragDetectListener ) {
//			lcdCanvas.removeDragDetectListener( canvasDragDetectListener );
//		}
		if (null != canvasMouseMoveListener) {
			lcdCanvas.removeMouseMoveListener(canvasMouseMoveListener);
		}

		if (null != canvasMouseListener) {
			lcdCanvas.removeMouseListener(canvasMouseListener);
		}

		if (null != canvasKeyListener) {
			lcdCanvas.removeKeyListener(canvasKeyListener);
		}

		if (null != canvasMenuDetectListener) {
			lcdCanvas.removeMenuDetectListener(canvasMenuDetectListener);
		}

		if (null != canvasFocusListener) {
			lcdCanvas.removeFocusListener(canvasFocusListener);
		}

		if (null != canvasMouseWheelListener) {
			lcdCanvas.removeMouseWheelListener(canvasMouseWheelListener);
		}
	}

	private Field getOSField( String field ) {

		String className = "";
		if (SwtUtil.isLinuxPlatform()) {
			className = GTK_OS_CLASS;
		} else if (SwtUtil.isWindowsPlatform()) {
			className = WIN32_OS_CLASS;
		} else if (SwtUtil.isMacPlatform()) {
			className = COCOA_OS_CLASS;
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
		if (SwtUtil.isLinuxPlatform()) {
			className = GTK_OS_CLASS;
		} else if (SwtUtil.isWindowsPlatform()) {
			className = WIN32_OS_CLASS;
		} else if (SwtUtil.isMacPlatform()) {
			className = COCOA_OS_CLASS;
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
				logger.warning( "GDK_WINDOWING_X11 returned null" );
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
				logger.warning( "GTK_WIDGET_WINDOW returned null" );
				return false;
			}

			Integer xWindow = (Integer) invokeOSMethod( getOSMethod( "gdk_x11_drawable_get_xid", int.class ),
					gtkWidgetWindow );
			if( null == xWindow ) {
				logger.warning( "gdk_x11_drawable_get_xid returned null" );
				return false;
			}

			Integer xDisplay = (Integer) invokeOSMethod( getOSMethod( "GDK_DISPLAY" ) );
			if( null == xDisplay ) {
				logger.warning( "GDK_DISPLAY returned null" );
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
				logger.warning( "xMessageAtomType is null" );
				return false;
			}

			Integer xMessageAtomAbove = (Integer) invokeOSMethod( xInternAtom, xDisplay, messageBufferAbove, false );
			if( null == xMessageAtomAbove ) {
				logger.warning( "xMessageAtomAbove is null" );
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
					logger.warning( "clientMessageField is null" );
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
						logger.warning( "topMost is null" );
						return false;
					}
					hWndInsertAfter = topMost.getInt( null );
				} else {
					Field noTopMost = getOSField( "HWND_NOTOPMOST" );
					if ( null == noTopMost ) {
						logger.warning( "HWND_NOTOPMOST is null" );
						return false;
					}
					hWndInsertAfter = noTopMost.getInt( null );
				}

				Field noSizeField = getOSField( "SWP_NOSIZE" );
				if ( null == noSizeField ) {
					logger.warning( "SWP_NOSIZE is null" );
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

			/* org.eclipse.swt.widgets.Shell */
			int shellHandle = 0;
			try {
				Field field = shell.getClass().getField("handle");
				shellHandle = field.getInt(shell);
				logger.info("shell.handle:" + shellHandle);
			} catch (IllegalArgumentException e) {
				logger.log(Level.SEVERE, e.getMessage(), e);
				shutdown();
			} catch (IllegalAccessException e) {
				logger.log(Level.SEVERE, e.getMessage(), e);
				shutdown();
			} catch (SecurityException e) {
				logger.log(Level.SEVERE, e.getMessage(), e);
				shutdown();
			} catch (NoSuchFieldException e) {
				logger.log(Level.SEVERE, e.getMessage(), e);
				shutdown();
			}

			invokeOSMethod( m, shellHandle, hWndInsertAfter, location.x, location.y, 0, 0, noSize );
		} else if( SwtUtil.isMacPlatform() ) {
			//TODO:
			logger.warning( "not supported yet" );
		}

		return true;
	}

	private boolean setTopMost64(boolean isOnTop)
	{
		if ( SwtUtil.isLinuxPlatform() ) {
			Boolean gdkWindowingX11 = (Boolean) invokeOSMethod( getOSMethod( "GDK_WINDOWING_X11" ) );
			if (null == gdkWindowingX11) {
				logger.warning( "GDK_WINDOWING_X11 returned null" );
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
				logger.warning( "GTK_WIDGET_WINDOW returned null" );
				return false;
			}

			Long xWindow = (Long) invokeOSMethod( getOSMethod( "gdk_x11_drawable_get_xid", long.class ),
					gtkWidgetWindow );
			if( null == xWindow ) {
				logger.warning( "gdk_x11_drawable_get_xid returned null" );
				return false;
			}

			Long xDisplay = (Long) invokeOSMethod( getOSMethod( "GDK_DISPLAY" ) );
			if( null == xDisplay ) {
				logger.warning( "GDK_DISPLAY returned null" );
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
				logger.warning( "xMessageAtomType is null" );
				return false;
			}

			Long xMessageAtomAbove = (Long) invokeOSMethod( xInternAtom, xDisplay, messageBufferAbove, false );
			if( null == xMessageAtomAbove ) {
				logger.warning( "xMessageAtomAbove is null" );
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
			Point location = shell.getLocation();

			long hWndInsertAfter = 0;
			int noSize = 0;

			try {
				if ( isOnTop ) {
					Field topMost = getOSField( "HWND_TOPMOST" );
					if ( null == topMost ) {
						logger.warning( "topMost is null" );
						return false;
					}
					hWndInsertAfter = topMost.getLong( null );
				} else {
					Field noTopMost = getOSField( "HWND_NOTOPMOST" );
					if ( null == noTopMost ) {
						logger.warning( "noTopMost is null" );
						return false;
					}
					hWndInsertAfter = noTopMost.getLong( null );
				}

				Field noSizeField = getOSField( "SWP_NOSIZE" );
				if ( null == noSizeField ) {
					logger.warning( "noSizeField is null" );
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

			Method m = getOSMethod( "SetWindowPos", long.class, long.class, int.class, int.class, int.class,
					int.class, int.class );

			/* org.eclipse.swt.widgets.Shell */
			long shellHandle = 0;
			try {
				Field field = shell.getClass().getField("handle");
				shellHandle = field.getLong(shell);
				logger.info("shell.handle:" + shellHandle);
			} catch (IllegalArgumentException e) {
				logger.log(Level.SEVERE, e.getMessage(), e);
				shutdown();
			} catch (IllegalAccessException e) {
				logger.log(Level.SEVERE, e.getMessage(), e);
				shutdown();
			} catch (SecurityException e) {
				logger.log(Level.SEVERE, e.getMessage(), e);
				shutdown();
			} catch (NoSuchFieldException e) {
				logger.log(Level.SEVERE, e.getMessage(), e);
				shutdown();
			}

			invokeOSMethod( m, shellHandle, hWndInsertAfter, location.x, location.y, 0, 0, noSize );
		} else if( SwtUtil.isMacPlatform() ) {
			//TODO:
			logger.warning("not supported yet");

			SkinUtil.openMessage(shell, null,
					"Sorry. This feature is not supported yet.",
					SWT.ICON_WARNING, config);
		}

		return true;
	}

	protected void openScreenShotWindow() {
		//TODO:
	}

	private void addMenuItems( final Shell shell, final Menu menu ) {

		/* Emulator detail info menu */
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

		/* Always on top menu */
		if (!SwtUtil.isMacPlatform()) { /* temp */

		final MenuItem onTopItem = new MenuItem( menu, SWT.CHECK );
		onTopItem.setText( "&Always On Top" );
		onTopItem.setSelection( isOnTop );

		onTopItem.addSelectionListener( new SelectionAdapter() {
			@Override
			public void widgetSelected( SelectionEvent e ) {

				final boolean isOnTop = onTopItem.getSelection();

				logger.info( "Select Always On Top : " + isOnTop );

				// readyToReopen( EmulatorSkin.this, isOnTop );

				/* internal/Library.java::arch() */
				String osArch = System.getProperty("os.arch"); //$NON-NLS-1$
				logger.info(osArch);
				if (osArch.equals("amd64") || osArch.equals("x86_64") ||
						osArch.equals("IA64W") || osArch.equals("ia64")) {
					//$NON-NLS-1$ $NON-NLS-2$ $NON-NLS-3$
					logger.info("64bit architecture");

					setTopMost64(isOnTop); /* 64bit */
				} else {
					logger.info("32bit architecture");

					setTopMost32(isOnTop);
				}
			}
		} );

		}

		/* Rotate menu */
		final MenuItem rotateItem = new MenuItem( menu, SWT.CASCADE );
		rotateItem.setText( "&Rotate" );
		rotateItem.setImage( imageRegistry.getIcon( IconName.ROTATE ) );
		Menu rotateMenu = createRotateMenu( menu.getShell() );
		rotateItem.setMenu( rotateMenu );

		/* Scale menu */
		final MenuItem scaleItem = new MenuItem( menu, SWT.CASCADE );
		scaleItem.setText( "&Scale" );
		scaleItem.setImage( imageRegistry.getIcon( IconName.SCALE ) );
		Menu scaleMenu = createScaleMenu( menu.getShell() );
		scaleItem.setMenu( scaleMenu );

		new MenuItem( menu, SWT.SEPARATOR );

		/* Advanced menu */
		final MenuItem advancedItem = new MenuItem( menu, SWT.CASCADE );
		advancedItem.setText( "Ad&vanced" );
		advancedItem.setImage( imageRegistry.getIcon( IconName.ADVANCED ) );
		Menu advancedMenu = createAdvancedMenu( menu.getShell() );
		advancedItem.setMenu( advancedMenu );

		/* Shell menu */
		final MenuItem shellItem = new MenuItem( menu, SWT.PUSH );
		shellItem.setText( "S&hell" );
		shellItem.setImage( imageRegistry.getIcon( IconName.SHELL ) );
		
		shellItem.addSelectionListener( new SelectionAdapter() {
			@Override
			public void widgetSelected( SelectionEvent e ) {
				if ( !communicator.isSensorDaemonStarted() ) {
					SkinUtil.openMessage(shell, null,
							"SDB is not ready.\nPlease wait until the emulator is completely boot up.",
							SWT.ICON_WARNING, config);
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

				if (SwtUtil.isLinuxPlatform()) {
					procSdb.command("/usr/bin/gnome-terminal", "--disable-factory",
							"--title=" + SkinUtil.makeEmulatorName( config ), "-x", sdbPath,
							"-s", "emulator-" + portSdb, "shell");
				} else if (SwtUtil.isWindowsPlatform()) {
					procSdb.command("cmd.exe", "/c", "start", sdbPath, "sdb",
							"-s", "emulator-" + portSdb, "shell");
				} else if (SwtUtil.isMacPlatform()) {
					procSdb.command("/usr/X11/bin/uxterm", "-T", "emulator-" + portSdb, "-e", sdbPath,"shell");
				}
				logger.log(Level.INFO, procSdb.command().toString());
			

				try {
					procSdb.start(); // open sdb shell
				} catch ( Exception ee ) {
					logger.log( Level.SEVERE, ee.getMessage(), ee );
					SkinUtil.openMessage( shell, null, "Fail to open Shell: \n" + ee.getMessage(), SWT.ICON_ERROR, config );
				}

				communicator.sendToQEMU( SendCommand.OPEN_SHELL, null );
			}
		} );

		new MenuItem( menu, SWT.SEPARATOR );

		/* Close menu */
		MenuItem closeItem = new MenuItem( menu, SWT.PUSH );
		closeItem.setText( "&Close" );
		closeItem.setImage( imageRegistry.getIcon( IconName.CLOSE ) );
		closeItem.addSelectionListener( new SelectionAdapter() {
			@Override
			public void widgetSelected( SelectionEvent e ) {
				logger.info("Close Menu is selected");
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

			if (currentState.getCurrentRotationId() == rotationId) {
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

					/* roll back a selection */
					item.setSelection( false );

					for ( MenuItem m : rotationList ) {
						short rotationId = (Short) m.getData();
						if (currentState.getCurrentRotationId() == rotationId) {
							m.setSelection( true );
							break;
						}
					}

					SkinUtil.openMessage(shell, null,
							"Rotation is not ready.\nPlease wait until the emulator is completely boot up.",
							SWT.ICON_WARNING, config);
					return;
				}

				short rotationId = ((Short) item.getData());

				skinComposer.arrangeSkin(currentState.getCurrentScale(), rotationId);

				LcdStateData lcdStateData =
						new LcdStateData(currentState.getCurrentScale(), rotationId);
				communicator.sendToQEMU(SendCommand.CHANGE_LCD_STATE, lcdStateData);
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

				int scale = ((Scale) item.getData()).value();

				skinComposer.arrangeSkin(scale, currentState.getCurrentRotationId());

				LcdStateData lcdStateData =
						new LcdStateData(scale, currentState.getCurrentRotationId());
				communicator.sendToQEMU(SendCommand.CHANGE_LCD_STATE, lcdStateData);

			}
		};
		
		for ( MenuItem menuItem : scaleList ) {

			int scale = ( (Scale) menuItem.getData() ).value();
			if (currentState.getCurrentScale() == scale) {
				menuItem.setSelection( true );
			}

			menuItem.addSelectionListener( selectionAdapter );

		}

		return menu;

	}

	private Menu createDiagnosisMenu(Shell shell) {
		Menu menu = new Menu(shell, SWT.DROP_DOWN);

		final MenuItem ramdumpItem = new MenuItem(menu, SWT.PUSH);
		ramdumpItem.setText("&Ram Dump");

		ramdumpItem.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				logger.info("Ram dump menu is selected");

				communicator.setRamdumpFlag(true);
				communicator.sendToQEMU(SendCommand.RAM_DUMP, null);

				RamdumpDialog ramdumpDialog;
				try {
					ramdumpDialog = new RamdumpDialog(EmulatorSkin.this.shell, communicator, config);
					ramdumpDialog.open();
				} catch (IOException ee) {
					logger.log( Level.SEVERE, ee.getMessage(), ee);
				}
			}
		});

		final MenuItem guestdumpItem = new MenuItem(menu, SWT.PUSH);
		guestdumpItem.setText("&Guest Memory Dump");

		guestdumpItem.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				logger.info("Guest memory dump menu is selected");

				communicator.setRamdumpFlag(true);
				communicator.sendToQEMU(SendCommand.GUEST_DUMP, null);
			}
		});

		return menu;
	}

	private Menu createAdvancedMenu(final Shell shell) {

		final Menu menu = new Menu(shell, SWT.DROP_DOWN);

		/* Control Panel menu */
		/*final MenuItem panelItem = new MenuItem(menu, SWT.PUSH);
		panelItem.setText("&Control Panel");
		//panelItem.setImage(imageRegistry.getIcon(IconName.XXX));
		panelItem.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				logger.info("Control Panel Menu is selected");

				if (controlPanel != null) {
					//TODO: move a window focus to controlPanel
					return;
				}

				RotationType rotation = SkinRotation.getRotation(currentRotationId);
				List<KeyMapType> keyMapList = rotation.getKeyMapList().getKeyMap();

				try {
					controlPanel = new ControlPanel(shell, communicator, keyMapList);
					controlPanel.open();
				} finally {
					controlPanel = null;
				}
			}
		} );*/

		/* Screen shot menu */
		final MenuItem screenshotItem = new MenuItem(menu, SWT.PUSH);
		screenshotItem.setText("&Screen Shot");
		screenshotItem.setImage(imageRegistry.getIcon(IconName.SCREENSHOT));
		screenshotItem.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				logger.info("ScreenShot Menu is selected");

				openScreenShotWindow();
			}
		} );

		/*
		// USB Keyboard menu
		final MenuItem usbKeyboardItem = new MenuItem(menu, SWT.CASCADE);
		usbKeyboardItem.setText("&USB Keyboard");
		usbKeyboardItem.setImage(imageRegistry.getIcon(IconName.USB_KEYBOARD));

		Menu usbKeyBoardMenu = new Menu(shell, SWT.DROP_DOWN);

		final MenuItem usbOnItem = new MenuItem(usbKeyBoardMenu, SWT.RADIO);
		usbOnItem.setText("On");
		usbOnItem.setSelection( isOnKbd );

		final MenuItem usbOffItem = new MenuItem(usbKeyBoardMenu, SWT.RADIO);
		usbOffItem.setText("Off");
		usbOffItem.setSelection(!isOnKbd);

		SelectionAdapter usbSelectionAdaptor = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				if (!communicator.isSensorDaemonStarted()) {
					SkinUtil.openMessage(shell, null,
							"USB is not ready.\nPlease wait until the emulator is completely boot up.",
							SWT.ICON_WARNING, config);
					usbOnItem.setSelection(isOnKbd);
					usbOffItem.setSelection(!isOnKbd);

					return;
				}

				MenuItem item = (MenuItem) e.getSource();
				if ( item.getSelection() ) {
					boolean on = item.equals(usbOnItem);
					isOnKbd = on;
					logger.info("USB keyboard " + isOnKbd);

					communicator.sendToQEMU(
							SendCommand.USB_KBD, new BooleanData(on, SendCommand.USB_KBD.toString()));
				}

			}
		};

		usbOnItem.addSelectionListener(usbSelectionAdaptor);
		usbOffItem.addSelectionListener(usbSelectionAdaptor);

		usbKeyboardItem.setMenu(usbKeyBoardMenu);
		*/

		// VirtIO Keyboard Menu
		final MenuItem hostKeyboardItem = new MenuItem(menu, SWT.CASCADE);
		hostKeyboardItem.setText("&Host Keyboard");
		hostKeyboardItem.setImage(imageRegistry.getIcon(IconName.HOST_KEYBOARD));

		Menu hostKeyboardMenu = new Menu(shell, SWT.DROP_DOWN);

		final MenuItem kbdOnItem = new MenuItem(hostKeyboardMenu, SWT.RADIO);
		kbdOnItem.setText("On");
		kbdOnItem.setSelection( isOnKbd );

		final MenuItem kbdOffItem = new MenuItem(hostKeyboardMenu, SWT.RADIO);
		kbdOffItem.setText("Off");
		kbdOffItem.setSelection(!isOnKbd);

		SelectionAdapter kbdSelectionAdaptor = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				if (!communicator.isSensorDaemonStarted()) {
					SkinUtil.openMessage(shell, null,
							"Host Keyboard is not ready.\nPlease wait until the emulator is completely boot up.",
							SWT.ICON_WARNING, config);
					kbdOnItem.setSelection(isOnKbd);
					kbdOffItem.setSelection(!isOnKbd);

					return;
				}

				MenuItem item = (MenuItem) e.getSource();
				if (item.getSelection()) {
					boolean on = item.equals(kbdOnItem);
					isOnKbd = on;
					logger.info("Host Keyboard " + isOnKbd);

					communicator.sendToQEMU(
							SendCommand.HOST_KBD, new BooleanData(on, SendCommand.HOST_KBD.toString()));
				}

			}
		};

		kbdOnItem.addSelectionListener(kbdSelectionAdaptor);
		kbdOffItem.addSelectionListener(kbdSelectionAdaptor);

		hostKeyboardItem.setMenu(hostKeyboardMenu);

		/* Diagnosis menu */
		if (SwtUtil.isLinuxPlatform()) { //TODO: windows
			final MenuItem diagnosisItem = new MenuItem(menu, SWT.CASCADE);
			diagnosisItem.setText("&Diagnosis");
			diagnosisItem.setImage(imageRegistry.getIcon(IconName.DIAGNOSIS));
			Menu diagnosisMenu = createDiagnosisMenu(menu.getShell());
			diagnosisItem.setMenu(diagnosisMenu);
		}

		new MenuItem(menu, SWT.SEPARATOR);

		/* About menu */
		final MenuItem aboutItem = new MenuItem(menu, SWT.PUSH);
		aboutItem.setText("&About");
		aboutItem.setImage(imageRegistry.getIcon(IconName.ABOUT));

		aboutItem.addSelectionListener(new SelectionAdapter() {
			private boolean isOpen;

			@Override
			public void widgetSelected(SelectionEvent e) {
				if (!isOpen) {
					isOpen = true;

					logger.info("Open the about dialog");
					AboutDialog dialog = new AboutDialog(shell);
					dialog.open();
					isOpen = false;
				}
			}
		} );

		new MenuItem(menu, SWT.SEPARATOR);

		/* Force close menu */
		final MenuItem forceCloseItem = new MenuItem(menu, SWT.PUSH);
		forceCloseItem.setText("&Force Close");
		forceCloseItem.setImage(imageRegistry.getIcon(IconName.FORCE_CLOSE));
		forceCloseItem.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				logger.info("Force close is selected");

				int answer = SkinUtil.openMessage(shell, null,
						"If you force stop an emulator, it may cause some problems.\n" +
						"Are you sure you want to contiue?",
						SWT.ICON_QUESTION | SWT.OK | SWT.CANCEL, config);

				if (answer == SWT.OK) {
					logger.info("force close!!!");
					System.exit(-1);
				}
			}
		});

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
		return currentState.getCurrentRotationId();
	}

}
