local httpapi = core.request_http_api_mainmenu_trusted()


local function asynctestfunc(param)
	print("paramparam " .. tostring(param))
	return "returnreturn"
end


-- NOTE: must come after async_event.lua to override its value
local stolen_job_list = {}
local handle_job_old = core.async_event_handler
local function handle_job_override(jobid, serialized_retval)
	print("jobhandler " .. tostring(serialized_retval))
	if stolen_job_list[jobid] then
		print("jobhandler_our")
		local retval = core.deserialize(serialized_retval)
		assert(type(core.async_jobs[jobid]) == "function")
		core.async_jobs[jobid](retval)
		stolen_job_list[jobid] = nil
		return
	end
	return handle_job_old(jobid, serialized_retval)
end
core.async_event_handler = handle_job_override


local function special_core_handle_async(func, parameter, callback)
	-- differs from core.handle_async by also using the stolen_job_list
	local serialized_func = string.dump(func)
	assert(serialized_func ~= nil)
	local serialized_param = core.serialize(parameter)
	if serialized_param == nil then return false end
	local jobid = core.do_async_callback(serialized_func, serialized_param)
	core.async_jobs[jobid] = callback
	stolen_job_list[jobid] = true		-- --
	return true
end


local function get_formspec(tabview, name, tabdata)
	local retval =
		"label[0.05,-0.25;".. fgettext("Online Users:") .. "]" ..
		"tablecolumns[color;tree;text]" ..
		"table[0,0.25;5.1,4.3;pkglist;" ..
		"abcd,efgh,ijkl" .. "]" ..
		"button[0,4.85;5.25,0.5;btn_contentdb;".. fgettext("Enable Discord Integration") .. "]"

	return retval
end


local function handle_buttons(tabview, fields, tabname, tabdata)
	if fields["pkglist"] ~= nil then
		local event = core.explode_table_event(fields["pkglist"])
		print("selected " .. tostring(event.row))
		return true
	end

	return false
end


local function on_change(evt, tab_src, tab_dst)
	if evt == "ENTER" then
		--core.handle_async(asynctestfunc, { a="a", b=4 }, function() end)
		special_core_handle_async(asynctestfunc, { a="a", b=4 }, function() end)
		if (httpapi) then
			local j = core.write_json({ hash=core.sha1(core.settings:get("friend_key")), action="userlist" })
			local handle = httpapi.fetch_async({ url="li1826-68.members.linode.com:5000/announce_user", post_data={ json=j } })
			local res = httpapi.fetch_async_get(handle)
			if res.completed then
				print("completed")
			else
				print("net")
			end
		end
	end
end


return {
	name = "friend",
	caption = fgettext("Friend"),
	cbf_formspec = get_formspec,
	cbf_button_handler = handle_buttons,
	on_change = on_change
}
