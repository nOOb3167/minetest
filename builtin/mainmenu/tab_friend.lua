local httpapi = core.request_http_api_mainmenu_trusted()
assert(httpapi)

gamedata.external_event_on_connect_data = ""

local auth_refresher = {
	auth_handles=nil,
	start=function (self)
		self.auth_handles = {}
	end,
	auth_request_for=function (self, destination, completion_cb)
		local j = core.write_json({ hash=core.sha1(core.settings:get("friend_key")), action="auth_issue", auth={ destination=destination } })
		local handle = httpapi.fetch_async({ url="li1826-68.members.linode.com:5000/announce_user", post_data={ json=j } })
		assert(type(completion_cb) == "function")
		assert(not self.auth_handles[destination])
		self.auth_handles[destination] = { http_handle=handle, token=nil, completion_cb=completion_cb }
	end,
	step=function (self)
		for k,v in pairs(self.auth_handles) do
			if v.http_handle then
				local res = httpapi.fetch_async_get(v.http_handle)
				if res.completed then
					v.http_handle = nil
					if res.succeeded and res.code == 200 then
						local json = core.parse_json(res.data)
						v.token = json.token
						print("completedAUTH " .. tostring(k) .. " : " .. dump(v))
					end
				end
			end
		end
	end,
}
auth_refresher:start()
auth_refresher:auth_request_for("http://example.com", function () end)

local userlist_refresher = {
	interval_refresh_ns=5000 * 1000,
	time_refresh=nil,
	http_handle=nil,
	userlist_data=nil,
	start=function (self)
		self.time_refresh = core.get_us_time()
		self.userlist_data = { userhash=nil, userlist={} }
		self:refresh()
	end,
	step=function (self)
		self:refresh()
		
		if self.http_handle then
			local res = httpapi.fetch_async_get(self.http_handle)
			if res.completed then
				self.http_handle = nil
				self.time_refresh = core.get_us_time() + self.interval_refresh_ns
				
				if res.succeeded and res.code == 200 then
					local json = core.parse_json(res.data)
					self.userlist_data = { userhash=json.userhash, userlist=json.userlist }
					print("completed1234 " .. dump(self.userlist_data))
				end
				
				ui.update()
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
	auth_refresher:step()
	core.handle_async(function (a) return "" end, "", asynctestfunc)
	return ""
end

------ simulate globalstep via async
core.handle_async(function (a) return "" end, "", asynctestfunc)


local function get_formspec(tabview, name, tabdata)
	local cells = ""
	for i,v in ipairs(userlist_refresher.userlist_data.userlist) do
		cells = cells .. "#00FF00," .. v .. ","
	end
	local retval =
		"label[0.05,-0.25;".. fgettext("Online Users:") .. "]" ..
		"tablecolumns[color;text]" ..
		"table[0,0.25;5.1,4.3;pkglist;" .. cells .. "]" ..
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
	end
end


return {
	name = "friend",
	caption = fgettext("Friend"),
	cbf_formspec = get_formspec,
	cbf_button_handler = handle_buttons,
	on_change = on_change
}
