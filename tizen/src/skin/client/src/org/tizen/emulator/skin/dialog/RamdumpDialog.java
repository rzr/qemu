package org.tizen.emulator.skin.dialog;

import java.io.IOException;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.SWTException;
import org.eclipse.swt.graphics.Cursor;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.ImageLoader;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.comm.sock.SocketCommunicator;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.util.SkinUtil;

public class RamdumpDialog extends SkinDialog {
	private SocketCommunicator communicator;
	private ImageData[] frames;

	private Logger logger = SkinLogger.getSkinLogger(RamdumpDialog.class).getLogger();

	public RamdumpDialog(Shell parent,
			SocketCommunicator communicator, EmulatorConfig config) throws IOException {
		super(parent, SkinUtil.makeEmulatorName(config),
				SWT.DIALOG_TRIM | SWT.APPLICATION_MODAL);
		this.communicator = communicator;
	}

	@Override
	protected Composite createArea(Composite parent) {
		Composite composite;

		try {
			composite = createContents(parent);
		} catch (IOException e) {
			logger.log(Level.SEVERE, e.getMessage(), e);
			return null;
		}

		RamdumpDialog.this.shell.addListener(SWT.Close, new Listener() {
			@Override
			public void handleEvent(Event event) {
				if (communicator.getRamdumpFlag() == true) {
					// do nothing
					logger.info("do nothing");

					event.doit = false;
				}
			}
		} );

		return composite;
	}

	private Composite createContents(Composite parent) throws IOException {
		Composite composite = new Composite(parent, SWT.NONE);

		composite.setLayout(new GridLayout(2, false));

		final Display display = Display.getDefault();

		ClassLoader classLoader = this.getClass().getClassLoader();
		ImageLoader loader = new ImageLoader();

		try {
			frames = loader.load(classLoader.getResourceAsStream("images/process.gif"));
		} catch (Exception e) {
			// TODO: register a indicator file
			frames = null;
		}

		final Label label = new Label(composite, SWT.NONE);
		if (frames != null) {
			label.setImage(new Image(display, frames[0]));
		}

		Label waitMsg = new Label(composite, SWT.NONE);
		waitMsg.setText("   Please wait...");

		Thread animation = new Thread() {
			int currentFrame = 0;
			boolean isDisposed = false;

			@Override
			public void run() {
				while (!isDisposed) {
					try {
						if (frames != null) {
							sleep(50);
						} else {
							sleep(500);
						}
					} catch (InterruptedException e) {
						logger.warning("InterruptedException");
					}

					if (frames != null) {
						currentFrame = (currentFrame + 1) % frames.length;
					}
					if (display.isDisposed()) {
						return;
					}

					if (frames != null) {
						display.asyncExec(new Runnable() {
							@Override
							public void run() {
								try {
									Image newImage = new Image(display, frames[currentFrame]);
									label.getImage().dispose();
									label.setImage(newImage);
								} catch (SWTException e) {
									isDisposed = true;
								}
							}
						}); //end of asyncExec
					}

					if (communicator.getRamdumpFlag() == false) {
						isDisposed = true;
					}
				}

				display.syncExec(new Runnable() {
					@Override
					public void run() {
						logger.info("close the Ramdump dialog");

						if (label.getImage() != null) {
							label.getImage().dispose();
						}
						shell.setCursor(null);
						RamdumpDialog.this.shell.close();
					}
				});
			}
		};

		shell.setCursor(display.getSystemCursor(SWT.CURSOR_WAIT));
		animation.start();

		return composite;
	}

	@Override
	protected void setShellSize() {
		shell.setSize(240, 110);
	}
}
