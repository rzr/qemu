package org.tizen.emulator.skin.dialog;

import java.io.IOException;
import java.io.InputStream;
import java.util.Properties;

import org.eclipse.swt.widgets.MessageBox;
import org.eclipse.swt.widgets.Shell;

public class AboutDialog {
	MessageBox messageBox;
	Properties properties;
	
	public AboutDialog(Shell shell) {
		if(!getProperties())
			return;
		
		messageBox = new MessageBox(shell);
		messageBox.setText("About");
		
		String message = makeMessage();
		messageBox.setMessage(message);
	}

	private String makeMessage() {
		String version = "Version : " + properties.getProperty("version");
		String buildTime = "Build time : " + properties.getProperty("build_time") + " (GMT)";
		String gitCommit = "Git : " + properties.getProperty("build_git_commit");
		
		return version + "\n" + buildTime + "\n" + gitCommit;
	}

	private boolean getProperties() {
		InputStream is = this.getClass().getClassLoader().getResourceAsStream("about.properties");
		if(is == null)
			return false;
		
		properties = new Properties();
		try {
			properties.load(is);
		} catch (IOException e) {
			e.printStackTrace();
			
			return false;
		}
		
		return true;
	}

	public void open() {
		messageBox.open();
	}
}
