#! /usr/bin/osascript
(*

 This scripot uses up3dtranscoder and up3dload from the UP3D project to transcode a g-code file and upload it to the printer.
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
 Next it will ask for the printer model and nozzle height. These parameters gets saved.
 
 
 How to use
 
 When you save or move or save files to the folder you configured for the script action, the transcoder and uploader
 will automatically start and ask for some essential settings (nozzle height). After processing the g-code file will
 be moved to a folder called processed. G-code files with the same name will be overwritten.
 
The script will only work with file name endings .gcode .gc .g .go
 
*)
use scripting additions
use framework "Foundation"
property NSArray : a reference to current application's NSArray


property extension_list : {"gcode", "gc", "g", "go"}
property model_list : {"mini", "classic", "plus", "box", "Cetus"}
property done_foldername : "processed"
property transcoder_path : ""
property uploader_path : ""
property nozzle_heights : {120.0, 120.0, 120.0, 120.0, 180.0} -- in the order of the model_list
property nozzle_limit : {min:100, max:310}


on adding folder items to this_folder after receiving added_items
	--display notification ("adding folder items: " & added_items)
	tell application "Finder"
		if not (exists folder done_foldername of this_folder) then
			make new folder at this_folder with properties {name:done_foldername}
		end if
	end tell
	run_as_app(added_items)
end adding folder items to

on launch (arguments)
	--display notification "app got launched" with title "print_up3d"
end launch

on open (arguments)
	--display notification ("open: " & arguments)
	set moveWhenDone to system attribute "moveWhenDone"
	process_arguments(arguments, moveWhenDone)
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
		set arguments to (choose file with prompt Â
			"UP3D select G-Code file to transcode and print" of type extension_list)
	end if
	process_arguments(arguments as list, "")
end run


on run_as_app(added_items)
	try
		-- we want to launch this script as an app in order to get the nice progress bar displayed
		-- the app must either be in the same directory or in the path eg /Applications
		set args to {}
		repeat with this_item in added_items
			set args to args & quoted form of (POSIX path of this_item) & " "
		end repeat
	end try
	try
		do shell script ("export moveWhenDone=1; open -a pup3d " & args)
	on error
		--error number -192 -- A resource wasnÕt found.	
		display dialog ("Missing pup3d.app. Please install pup3d.app inside /Applications folder.") with title "Error" buttons {"Cancel"}
	end try
end run_as_app


on process_arguments(arguments, moveWhenDone)
	-- first we filter the g-code files
	set filtered_items to {}
	repeat with this_item in arguments
		set item_info to the info for this_item
		if (the name extension of the item_info is in the extension_list) then
			set filtered_items to filtered_items & this_item
		end if
	end repeat
	
	-- with a single file given we just run it
	-- with multiple files we need to pop up an selector since there is no print queue
	-- the user can then select one by one file to print
	if number of items in filtered_items is equal to 1 then
		process(first item of filtered_items)
		if moveWhenDone = "1" then
			moveWhenDone(first item of filtered_items)
		end if
	else if number of items in filtered_items is greater than 1 then
		repeat
			set this_item to choose from list filtered_items
			if this_item is false then exit repeat
			process(this_item)
			if moveWhenDone = "1" then
				moveWhenDone(this_item)
			end if
		end repeat
	else
		set extns to {}
		repeat with this_item in extension_list
			set extns to extns & this_item & " "
		end repeat
		display notification ("Only g-code files with extensions " & extns & "are supported.")
	end if
end process_arguments


-- here we move the gcode file to the done_folder previosuely created when adding folder items
on moveWhenDone(this_item)
	set this_file to this_item as alias
	tell application "Finder"
		set destination to (parent of this_file as text) & done_foldername
		if not (exists destination) then
			set destination to make new folder at parent of this_file with properties {name:done_foldername}
		end if
		move this_file to destination with replacing
	end tell
end moveWhenDone


-- process a G-code file
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

on getHeight(model)
	set num to its getIndexOfItem:(model as string) inList:model_list
	set height to get item num of nozzle_heights
	return height
end getHeight

-- get printer model from defaults. If not defined then ask user for it.
on getPrinterModel(forceToAsk)
	if forceToAsk is true then
		set model to "ask"
	else
		try
			set model to do shell script ("defaults read com.up3d.transcode model")
		on error
			set model to "ask"
		end try
	end if
	if model is equal to "ask" then
		set model to choose from list model_list
		if model is false then
			-- exit with "User Canceled" 
			error number -128
		end if
		do shell script ("defaults write com.up3d.transcode model " & model)
	end if
	return model
end getPrinterModel


on getTranscoder()
	if transcoder_path = "" then
		set transcoder_path to POSIX path of Â
			(choose file with prompt Â
				"Can't find transcoder executable. Please select UP3D Transcoder." of type {"public.executable"})
	end if
	return transcoder_path
end getTranscoder

on getUploader()
	if uploader_path = "" then
		set uploader_path to POSIX path of (choose file with prompt Â
			"Can't find uploader executeable. Please Select UP3D uploader" of type {"public.executable"})
	end if
	return uploader_path
end getUploader

on transcode(filename)
	set ptmpTranscode to POSIX path of (path to temporary items from user domain) & "transcode.umc"
	set model to getPrinterModel(false)
	-- ask user for nozzle height
	repeat
		set height to getHeight(model)
		set DlogResult to display dialog ("Printing: " & POSIX path of filename & linefeed & "Printer: " & model & linefeed & "Set Nozzle Height:") Â
			default answer height Â
			buttons {"Cancel", "Change Model", "Transcode"} default button 3 Â
			with title "UP3D Transcoding from G-Code"
		try
			set height to text returned of DlogResult as real
		on error
			set height to 0
		end try
		set answer to button returned of DlogResult
		if answer is equal to "Cancel" then
			return {}
		end if
		if answer is equal to "Change Model" then
			set model to getPrinterModel(true)
			set height to getHeight(model)
		else
			-- the Cetus3D can go up to 300mm on Z-axis
			if height < min of nozzle_limit or height > max of nozzle_limit then
				display dialog "Nozzle Height must be set between " & min of nozzle_limit & " and " & max of nozzle_limit & " mm."
			else
				exit repeat
			end if
		end if
	end repeat
	set transcoder to getTranscoder()
	-- save nozzle height to defaults
	--do shell script ("defaults write com.up3d.transcode nozzle_height_" & model & " " & height)
	--	try
	--		set nozzle_heights to nozzle_heights whose name is not model
	--	end try
	--	make new property list item at the end of property list items of contens of nozzle_heights with properties {name:} 
	
	set num to its getIndexOfItem:(model as string) inList:model_list
	set item num in nozzle_heights to height
	
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


(*
-- convert pair of strings into a record
-- http://books.gigatux.nl/mirror/applescriptdefinitiveguide/applescpttdg2-CHP-14-SECT-6.html#applescpttdg2-CHP-14-SECT-6
on listToRecord(L)
	script myScript
		return {Çclass usrfÈ:L}
	end script
	return run script myScript
end listToRecord
*)

(*
on getWhat(what, r)
	script myScript
		return get what of r
	end script
	return run script myScript
end getWhat
*)

(*
http://applehelpwriter.com/2016/08/09/applescript-get-item-number-of-list-item/
*)
on getIndexOfItem:anItem inList:aList
	set anArray to NSArray's arrayWithArray:aList
	set ind to ((anArray's indexOfObject:anItem) as number) + 1 # see note below
	if ind is greater than (count of aList) then
		--display dialog "Item '" & anItem & "' not found in list." buttons "OK" default button "OK" with icon 2 with title "Error"
		return 0
	else
		return ind
	end if
end getIndexOfItem:inList: