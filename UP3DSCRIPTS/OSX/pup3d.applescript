#! /usr/bin/osascript
(*

 This scripot uses up3dtranscoder and up3dload from the UP3D project to transcode a gcode file and upload it to the printer.
 It allows you to automatically transcode the g-code and send it to the 3d printer.
 When saving the g-code from your slicer (like Slic3r, Cura or Simplify3D) to a specific folder
 the script starts.
 
 Installation
 
 Open this script via Apple Script-Editor and export it as an program (pup3d.app)
 Move pup3d.app to the /Applications folder

 In order to make it available for action folder it needs to export this script as an script (pup3d.scpt)
 in the Apple Script Editor and then move it with the Finder to the final destination:
 /Library/Scripts/Folder Action Scripts/
 
 The Finder will ask you for your password to confirm the move.
 You now have two versions of this script, pup3d.app and pup3d.scpt installed in your system.
 
 Next is to configure the folder actions. Follow these steps:
 - create a folder where you will place your g-code. Name it e.g. 3D_print_jobs (whatever name and location suits you).
 - right click on the folder and select Services (Dienste) /  Folder Actions configure ... (Ordneraktionen konfigurieren ...)
 - select the pup3d.scpt you just move to the Folder Action Scripts
 
 When you first launch the script it will ask you to point to the up3dtranscoder and up3dloader program.
 Next it will ask for the printer model and nozzle height. These parameters gets saved in the defaults of you mac.
 You can look up the setting in terminal with this command:

 defaults read up3d.co

 and you can reset the defaults if you wish with the follwing command:
 
 defaults delete up3d.com
 
 
 How to use
 
 When you save or move files to the folder you configured for the script action the transcoder and uploader
 will automatically start and ask for some essential settings (nozzle height)
 
The script will only work with file name endings .gcode .gc .g .go
 
*)

property extension_list : {"gcode", "gc", "g", "go"}

on adding folder items to this_folder after receiving added_items
	--display notification "adding folder items"
	try
		if number of items in added_items > 0 then
			repeat with this_item in added_items
				set item_info to the info for this_item
				if (the name extension of the item_info is in the extension_list) then
					set appInstalled to false
					try
						-- we want to launch this script as an app in order to get the nice progress bar displayed
						-- the app must either be in the same directory or in the path eg /Applications
						do shell script ("open -a pup3d " & quoted form of (POSIX path of this_item))
						set appInstalled to true
					end try
					if appInstalled is not true then
						error number -192 -- A resource wasnÕt found.	
						-- no app, no progress bar :(
						display dialog ("Missing pup3d.app. Please install pup3d.app inside /Applications folder.") with title "Error" buttons {"Cancel"}
						--process(this_item)
					end if
					tell application "Finder" to delete file this_item
				end if
			end repeat
		end if
	on error thisErr
		display notification thisErr
	end try
end adding folder items to

on launch (arguments)
	--display notification "app got launched" with title "print_up3d"
end launch

on open (arguments)
	--display notification "open"
	if number of items in arguments > 0 then
		set fa to first item of arguments
		process(fa)
	else
		--display notification "nothing to do" with title "print_up3d"
	end if
end open

on run (arguments)
	--display notification "run" with title "print_up3d"
	try
		set argc to number of items in arguments
	on error
		set argc to 0
	end try
	
	-- if no arguments are present we ask for a file
	if argc = 0 then
		set fname to (choose file with prompt Â
			"UP3D select G-Code file to transcode and print" of type extension_list)
	else
		set fname to (first item of arguments)
	end if
	
	process(fname)
end run


-- process a G-code file given in 
on process(gcode)
	set transcoderResult to transcode(gcode)
	if number of items in transcoderResult > 0 then
		display dialog (status of transcoderResult) with title Â
			"UP3D transcoding result" buttons {"Cancel", "Send To Printer"} default button 2
		if button returned of result = "Send To Printer" then
			upload(tmpFile of transcoderResult)
		end if
		try
			-- clean up tmp file
			do shell script ("rm -rf " & quoted form of tmpFile of transcoderResult)
		end try
	end if
end process

on getHeight()
	try
		set height to do shell script "defaults read com.up3d.transcode nozzle_height"
	on error
		set height to 123.45
	end try
	return height
end getHeight

-- get printer model from defaults. If not defined then ask user for it.
on getPrinterModel()
	set model to "Mini"
	try
		set model to do shell script ("defaults read com.up3d.transcode model")
	on error
		repeat
			set DlogResult to display dialog ("Please type UP printer model:") Â
				default answer model Â
				buttons {"Cancel", "OK"} default button 1 Â
				with title "UP3D Transcoder"
			set answer to button returned of DlogResult
			set model to text returned of DlogResult as text
			if answer is equal to "Cancel" then
				-- exit with "User Canceled" 
				error number -128
			end if
			-- now we use tr to translate the model name to lower case
			set the model to do shell script ("echo " & quoted form of (model) & " | tr A-Z a-z")
			if {"mini", "classic", "plus", "box", "Cetus"} contains model then
				exit repeat
			else
				display dialog ("Unknown UP printer model. Known models are mini, classic, plus, box, Cetus.")
			end if
		end repeat
		if model is equal to "cetus" then set model to "Cetus" -- Cetus needs to be captial  
		do shell script ("defaults write com.up3d.transcode model " & model)
	end try
	return model
end getPrinterModel


on getTranscoder()
	try
		set transcoder to do shell script "defaults read com.up3d.transcode transcoder_path"
	on error
		set transcoder to POSIX path of Â
			(choose file with prompt Â
				"Can't find transcoder executable. Please select UP3D Transcoder." of type {"public.executable"})
		do shell script ("defaults write com.up3d.transcode transcoder_path " & quoted form of transcoder)
	end try
	return transcoder
end getTranscoder

on getUploader()
	try
		set uploader to do shell script "defaults read com.up3d.transcode uploader_path"
	on error
		set uploader to POSIX path of (choose file with prompt Â
			"Can't find uploader executeable. Please Select UP3D uploader" of type {"public.executable"})
		do shell script ("defaults write com.up3d.transcode uploader_path " & quoted form of uploader)
	end try
	return uploader
end getUploader

on transcode(filename)
	set ptmpTranscode to POSIX path of (path to temporary items from user domain) & "transcode.umc"
	set model to getPrinterModel()
	set height to getHeight()
	-- ask user for nozzle height
	repeat
		set DlogResult to display dialog ("Printing: " & POSIX path of filename & linefeed & "Printer: " & model & linefeed & "Set Nozzle Height:") Â
			default answer height Â
			buttons {"Cancel", "Transcode"} default button 2 Â
			with title "UP3D Transcoding from G-Code"
		set height to text returned of DlogResult as real
		set answer to button returned of DlogResult
		if answer is equal to "Cancel" then
			return {}
		end if
		if height < 100 or height > 200 then
			display dialog "Nozzle Height must be set between 100 and 200 mm."
		else
			exit repeat
		end if
	end repeat
	set transcoder to getTranscoder()
	-- save nozzle height to defaults
	do shell script ("defaults write com.up3d.transcode nozzle_height " & height)
	set nozzle_height to height as text
	-- replace comma to point in nozzle height string
	set o to offset of "," in nozzle_height
	if o is not 0 then set nozzle_height to text 1 thru (o - 1) of nozzle_height & "." & text (o + 1) thru -1 of nozzle_height
	do shell script (quoted form of transcoder & " " & model & "  " & quoted form of POSIX path of filename & Â
		" " & quoted form of ptmpTranscode & " " & nozzle_height)
	return {tmpFile:ptmpTranscode, status:result}
end transcode

on upload(pfilename)
	set ptmpUpload to POSIX path of (path to temporary items from user domain) & "upload.out"
	set uploader to getUploader()
	set retry to false
	repeat
		do shell script (quoted form of uploader & " " & quoted form of pfilename & Â
			" > " & quoted form of ptmpUpload & " 2>&1 &")
		log ("upload launched...")
		set progress description to "Sending data to printer"
		set progress additional description to "UploadingÉ"
		set progress total steps to 100
		repeat
			--delay 0.2
			try
				-- now we get the last line of the uploader output
				-- first we strip newlines from the file
				-- then we translate carriage returns to newlines
				-- finally we can get the last line with tail
				do shell script ("tr -d '\\n' < " & quoted form of ptmpUpload & " | tr '\\r' '\\n' | tail -n 1")
				set status_line to result
				log (status_line)
				if status_line contains "ERROR" then
					display dialog status_line with title Â
						"UP3D upload" buttons {"Cancel", "Retry"} default button 2
					if button returned of result = "Retry" then
						set retry to true
					else
						set retry to false
					end if
					exit repeat
				else if (count of words of status_line) > 5 then
					try
						set p to item 5 of words of status_line as number
						--set progress additional description to status_line
						set progress completed steps to p
						log (p)
						if p is equal to 100 then
							retry = false
							exit repeat
						end if
					on error thisErr
						log ("fail: " & (words of status_line) & " | " & thisErr)
					end try
				end if
			on error thisErr
				display alert thisErr
				return
			end try
		end repeat
		if retry = false then exit repeat
	end repeat
	-- clean up tmp file
	try
		do shell script ("rm -rf " & quoted form of ptmpUpload)
	end try
end upload
