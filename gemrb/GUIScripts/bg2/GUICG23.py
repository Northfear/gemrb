#character generation, biography (GUICG23)
import GemRB

BioWindow = 0
EditControl = 0

def OnLoad ():
	global BioWindow, EditControl

	GemRB.LoadWindowPack ("GUICG", 640, 480)
	BioWindow = GemRB.LoadWindowObject (23)

	EditControl = BioWindow.GetControl (3)
	BIO = GemRB.GetToken("BIO")
	EditControl = GemRB.ConvertEdit (BioWindow, EditControl, 5)
	EditControl.SetVarAssoc ("row", 0)
	if BIO:
		EditControl.SetText (BIO)
	else:
		EditControl.SetText (15882)

	# done
	OkButton = BioWindow.GetControl (1)
	OkButton.SetText (11973)

	ClearButton = BioWindow.GetControl (4)
	ClearButton.SetText (34881)

	# back
	CancelButton = BioWindow.GetControl (2)
	CancelButton.SetText (12896)

	OkButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OkPress")
	ClearButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "ClearPress")
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	BioWindow.SetVisible (1)
	return

def OkPress ():
	global BioWindow, EditControl

	row = 0
	line = None
	BioData = ""

	#there is no way to get the entire TextArea content
	#this hack retrieves the TextArea content row by row
	#there is no way to know how much data is in the TextArea
	while 1:
		GemRB.SetVar ("row", row)
		EditControl.SetVarAssoc ("row", row)
		line = EditControl.QueryText ()
		if len(line)<=0:
			break
		BioData += line+"\n"
		row += 1
	
	if BioWindow:
		BioWindow.Unload ()
	GemRB.SetNextScript ("CharGen9")
	GemRB.SetToken ("BIO", BioData)
	return
	
def CancelPress ():
	if BioWindow:
		BioWindow.Unload ()
	GemRB.SetNextScript ("CharGen9")
	return

def ClearPress ():
	EditControl.SetText ("")
	return
