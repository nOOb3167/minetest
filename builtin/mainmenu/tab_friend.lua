local httpapi = core.request_http_api_mainmenu_trusted()
assert(httpapi)

local userlist_refresher = {
	interval_refresh_ns=5000 * 1000,
	time_refresh=nil,
	http_handle=nil,
	start=function (self)
		self.time_refresh = core.get_us_time()
		self:refresh()
	end,
	step=function (self)
		self:refresh()
		
		if self.http_handle then
			local res = httpapi.fetch_async_get(self.http_handle)
			if res.completed then
				self.http_handle = nil
				self.time_refresh = core.get_us_time() + self.interval_refresh_ns
				
				print("completed1234")
			end
		end
	end,
	refresh=function (self)
			if core.get_us_time() >= self.time_refresh and not self.http_handle then
				local j = core.write_json({ hash=core.sha1(core.settings:get("friend_key")), action="userlist" })
				local handle = httpapi.fetch_async({ url="li1826-68.members.linode.com:5000/announce_user", post_data={ json=j } })
				self.http_handle = handle
			end
		end
}
userlist_refresher:start()

local function asynctestfunc(param)
	userlist_refresher:step()
	core.handle_async(function (a) return "" end, "", asynctestfunc)
	return ""
end

------ simulate globalstep via async
core.handle_async(function (a) return "" end, "", asynctestfunc)


local n = 0
local function get_formspec(tabview, name, tabdata)
	local retval =
		"label[0.05,-0.25;".. fgettext("Online Users:") .. "]" ..
		"tablecolumns[color;tree;text]" ..
		"table[0,0.25;5.1,4.3;pkglist;" ..
		"abcd,efgh,ijkl" .. tostring(n)  .. "]" ..
		"button[0,4.85;5.25,0.5;btn_contentdb;".. fgettext("Enable Discord Integration") .. "]"
	n = n + 1
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
	end
end


return {
	name = "friend",
	caption = fgettext("Friend"),
	cbf_formspec = get_formspec,
	cbf_button_handler = handle_buttons,
	on_change = on_change
}
