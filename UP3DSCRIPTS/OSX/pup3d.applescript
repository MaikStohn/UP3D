#! /usr/bin/osascript
(*

 This scripot uses up3dtranscoder and up3dload from the UP3D project to transcode a g-code file and upload it to the printer.
 It allows you to automatically transcode the g-code and send it to the 3d printer.
 When saving the g-code from your slicer (like Slic3r, Cura or Simplify3D) to a specific folder
 the script starts.
 
 Installation
 
 Open this script via Apple Script-Editor and export it as an program (pup3d.app)
 Move pup3d.app to the /Applications folder
 Next the up3dtranscode and up3dload executabels need to be moved to the app package.
 This can be done by right clock on the pup3d.app and select show package content. You
 then need to copy the two files to the Contents/MacOS folder.
 

 In order to make it available for action folder it needs to export this script as an script (pup3d.scpt)
 in the Apple Script Editor and then move it with the Finder to the final destination:
 /Library/Scripts/Folder Action Scripts/
 
 The Finder will ask you for your password to confirm the move.
 You now have two versions of this script, pup3d.app and pup3d.scpt installed in your system.
 
 Next is to configure the folder actions. Follow these steps:
 - create a folder where you will place your g-code. Name it e.g. 3D_print_jobs (whatever name and location suits you).
 - right click on the folder and select Services (Dienste) /  Folder Actions configure ... (Ordneraktionen konfigurieren ...)
 - select the pup3d.scpt you just move to the Folder Action Scripts
 
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
property nozzle_heights : {123.45, 123.45, 123.45, 123.45, 180.12} -- in the order of the model_list
property nozzle_limit : {min:100.0, max:310.0}
property current_model : "--- please select printer type ---"

global moveWhenDone

on adding folder items to this_folder after receiving added_items
	--display notification ("adding folder items: " & added_items)
	set moveWhenDone to "1"
	run_as_app(added_items)
end adding folder items to

on launch (arguments)
	--display notification "app got launched" with title "print_up3d"
end launch

on open (arguments)
	--display notification ("open: " & arguments)
	try
		set moveWhenDone to system attribute "moveWhenDone"
	on error
		set moveWhenDone to "0"
	end try
	process_arguments(arguments)
end open

on run (arguments)
	--display notification "run" with title "print_up3d"
	try
		set argc to number of items in arguments
	on error
		set argc to 0
	end try
	
	set moveWhenDone to "0"
	
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
		do shell script ("export moveWhenDone=" & moveWhenDone & "; open -n -a pup3d " & args)
	on error
		--error number -192 -- A resource wasnÕt found.	
		display dialog ("Missing pup3d.app. Please install pup3d.app inside /Applications folder.") with title "Error" buttons {"Cancel"}
	end try
end run_as_app


on process_arguments(arguments)
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
	else if number of items in filtered_items is greater than 1 then
		set this_item to choose from list filtered_items with title "UP3D" with prompt "Select file to print:"
		if this_item is not false then
			process(this_item)
			-- in case the file got moved we remove it from the list
			if moveWhenDone is "1" then
				set next_arguments to {}
				repeat with next_item in filtered_items
					if next_item is not equal to this_item then
						set next_arguments to next_arguments & next_item
					end if
				end repeat
				set filtered_items to next_arguments
			end if
			
			-- in order to get the progress window close
			-- we respawn the program with the same list and return
			if number of items in filtered_items > 0 then
				run_as_app(filtered_items)
			end if
		end if
	else -- no item matches the criteria
		set extns to {}
		repeat with this_item in extension_list
			set extns to extns & this_item & " "
		end repeat
		display notification ("Only g-code files with extensions " & extns & "are supported.")
	end if
end process_arguments


-- here we move the gcode file to the done_folder previosuely created when adding folder items
on moveFileToProcessedFolder(this_item)
	if moveWhenDone = "1" then
		set this_file to this_item as alias
		tell application "Finder"
			set destination to (parent of this_file as text) & done_foldername
			if not (exists destination) then
				set destination to make new folder at parent of this_file with properties {name:done_foldername}
			end if
			move this_file to destination with replacing
		end tell
	end if
end moveFileToProcessedFolder


-- process a G-code file
on process(gcode)
	set transcoderResult to transcode(gcode)
	if transcoderResult is not {} then
		display dialog (status of transcoderResult) with title Â
			"UP3D transcoding result" buttons {"Cancel", "Send To Printer"} default button 2
		if button returned of result = "Send To Printer" then
			upload(tmpFile of transcoderResult)
			moveFileToProcessedFolder(gcode)
		end if
		try
			-- clean up tmp file
			do shell script ("rm -rf " & quoted form of tmpFile of transcoderResult)
		end try
	end if
end process


-- get nozzle height of current model
-- if model is not set then return as default the first parameter of nozzle_heights
on getHeight(model)
	set num to its getIndexOfItem:(model as string) inList:model_list
	if num = 0 then set num to 1
	set height to get item num of nozzle_heights
	return height
end getHeight

-- get printer model from defaults. If not defined then ask user for it.
on askPrinterModel()
	set model to choose from list model_list with title "PUP3D" with prompt "Select printer type:"
	if model is not {} and model is not false then
		set current_model to model
	end if
	return current_model
end askPrinterModel


on getTranscoder()
	if transcoder_path = "" then
		set my_path to POSIX path of (path to me as text) & "Contents/MacOS/up3dtranscode"
		try
			do shell script ("ls " & quoted form of my_path)
		on error
			display alert ("Missing the 'up3dtranscode' in this bundle.")
			error number -192
		end try
		set transcoder_path to my_path
	end if
	return transcoder_path
end getTranscoder

on getUploader()
	if uploader_path = "" then
		set my_path to POSIX path of (path to me) & "Contents/MacOS/up3dload"
		try
			do shell script ("ls " & quoted form of my_path)
		on error
			display alert ("Missing 'up3dloade' in this bundle.")
			error number -192
		end try
		set uploader_path to my_path
	end if
	return uploader_path
end getUploader


-- transcodes a given g-code file and asks for printer type and nozzle height
-- on success it returns the path to the transcoded file
-- on fail it returns {}
on transcode(filename)
	set ptmpTranscode to POSIX path of (path to temporary items from user domain) & "transcode.umc"
	-- ask user for nozzle height
	repeat
		set height to getHeight(current_model)
		set DlogResult to display dialog ("Printing: " & POSIX path of filename & linefeed & "Printer: " & current_model & linefeed & "Set Nozzle Height:") Â
			default answer height Â
			buttons {"Cancel", "Select Printer Type", "Transcode..."} default button 3 Â
			with title "UP3D Transcoding from G-Code"
		try
			set height to text returned of DlogResult as number
		on error
			set height to 0
		end try
		set answer to button returned of DlogResult
		if answer is equal to "Cancel" then
			return {}
		end if
		if answer is equal to "Select Printer Type" or current_model is not in model_list then
			askPrinterModel()
			set height to getHeight(current_model)
		else
			-- the Cetus3D can go up to 300mm on Z-axis
			if height < min of nozzle_limit or height > max of nozzle_limit then
				display alert "Nozzle Height must be set between " & min of nozzle_limit & " and " & max of nozzle_limit & " mm."
			else
				exit repeat
			end if
		end if
	end repeat
	-- save nozzle height in property nozzle_heights
	set num to its getIndexOfItem:(current_model as string) inList:model_list
	set item num in nozzle_heights to height
	-- ask for the transcoder path
	set transcoder to getTranscoder()
	set nozzle_height to height as text
	-- replace comma to point in nozzle height string
	set o to offset of "," in nozzle_height
	if o is not 0 then set nozzle_height to text 1 thru (o - 1) of nozzle_height & "." & text (o + 1) thru -1 of nozzle_height
	do shell script (quoted form of transcoder & " " & current_model & "  " & quoted form of POSIX path of filename & Â
		" " & quoted form of ptmpTranscode & " " & nozzle_height)
	return {tmpFile:ptmpTranscode, status:result}
end transcode

on upload(pfilename)
	set ptmpUpload to POSIX path of (path to temporary items from user domain) & "upload.out"
	set uploader to getUploader()
	repeat
		set retry to true
		do shell script (quoted form of uploader & " " & quoted form of pfilename & Â
			" &> " & quoted form of ptmpUpload & "  & echo $!")
		set pid to result
		log ("upload launched with pid: " & pid)
		set progress description to "Sending data to printer"
		set progress additional description to "start uploading"
		set progress total steps to 100
		set p to 0
		set t to (time of (current date)) --get start time in seconds
		set file_size to 0
		repeat
			--delay 0.2  --slowing down the polling of the log tail decreases the chance to get a completet line
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
					if button returned of result is equal to "Retry" then
						set retry to true
					else
						set retry to false
					end if
					exit repeat
				else if (count of words of status_line) > 5 then
					try
						set pn to item 5 of words of status_line as number
					on error thisErr
						log ("fail: " & (words of status_line) & " | " & thisErr)
					end try
					if pn > p then
						set p to pn
						set progress additional description to "uploading..."
						set progress completed steps to p
						if p is equal to 100 then
							--set retry to false
							set progress additional description to "upload completed"
							--exit repeat
						end if
					end if
				end if
				-- now we check if the upload process still runs
				try
					do shell script ("kill -0 " & pid)
					-- process is still alive here
				on error
					-- the process has ended, so we get an error from the above kill -0 pid command
					-- only if 100% has benn uploaded without error then we allow to exit
					if p is equal to 100 then
						set retry to false
					end if
					exit repeat
				end try
				-- t is always set to the last time the output file size increments 
				set current_size to do shell script ("stat -f%z " & quoted form of ptmpUpload)
				if current_size > file_size then
					set file_size to current_size
					set t to (time of (current date))
				end if
				-- check for time out
				log ("timeout conter: " & (time of (current date)) - t)
				if (time of (current date)) - t > 10 then
					set progress additional description to "timeout during upload"
					display alert ("Could not complete upload in time." & linefeed & status_line)
					set retry to true
					exit repeat
				end if
			on error thisErr
				display alert thisErr
				return
			end try
		end repeat
		try
			-- clean up process in case anything hangs here
			do shell script ("kill -9 " & pid)
		end try
		try
			-- clean up tmp file
			do shell script ("rm -rf " & quoted form of ptmpUpload)
		end try
		if retry is not true then exit repeat
	end repeat
	-- let user know the status of the upload
	set progress additional description to status_line
	delay 2
	-- clean up progress
	set progress total steps to -1
	set progress completed steps to -1
	--set progress description to ""
	--set progress additional description to ""
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