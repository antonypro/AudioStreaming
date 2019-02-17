(
function(widget) 
{
	if (widget == "button_delete")
	{
		var result = settings.question("Delete account", "Are you sure to DELETE PERMANENTLY THIS ACCOUNT?");
		
		if (result)
			settings.write("DELETE_ACCOUNT");
	}
	else if (widget == "button_new_password")
	{
		var pass = settings.readProperty("edit_new_password", "text");
		var pass_repeat = settings.readProperty("edit_new_password_repeat", "text");
		
		settings.writeProperty("edit_new_password", "text", "");
		settings.writeProperty("edit_new_password_repeat", "text", "");
		
		if (pass == "" || pass_repeat == "")
		{
			settings.error("Error", "Please type the new password!");
			return;
		}
		
		if (pass != pass_repeat)
		{
			settings.error("Error", "Password doesn't match!");
			return;
		}
		
		var result = settings.question("Change password", "Are you sure to PERMANENTLY CHANGE THIS ACCOUNT PASSWORD?");
		
		if (result)
			settings.write("CHANGE_ACCOUNT_PASSWORD", pass);
	}
}
)
