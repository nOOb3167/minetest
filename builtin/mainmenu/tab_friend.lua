local httpapi = core.request_http_api_mainmenu_trusted()
assert(httpapi)

local http_helper = {
	interval_refresh_us=nil,
	xself=nil,
	issue_request_cb=nil,
	completion_cb=nil,
	time_refresh=nil,
	http_handle=nil,
	start=function (http_helper, interval_refresh_us, xself, issue_request_cb, completion_cb)
		local self = table.copy(http_helper)
		
		self.interval_refresh_us = interval_refresh_us
		self.xself = xself
		self.issue_request_cb = issue_request_cb
		self.completion_cb = completion_cb
		self.time_refresh = core.get_us_time()
		self.http_handle = nil
		
		self:refresh()
		
		return self
	end,
	step=function (self)
		self:refresh()
		
		if self.http_handle then
			local res = httpapi.fetch_async_get(self.http_handle)
			if res.completed then
				self.http_handle = nil
				self.time_refresh = core.get_us_time() + self.interval_refresh_us
				local success = res.succeeded and res.code == 200
				local json = success and core.parse_json(res.data) or nil
				self.xself:completion_cb(success, json)
			end
		end
	end,
	refresh=function (self)
		if core.get_us_time() >= self.time_refresh and not self.http_handle then
			self.http_handle = self.xself:issue_request_cb()
		end	
	end,
}

local party_refresher = {
	party_data=nil,
	issue_request_cb=function (self)
		print("ISSUING")
		local j = core.write_json({ hash=core.sha1(core.settings:get("friend_key")), action="partylist" })
		local handle = httpapi.fetch_async({ url="li1826-68.members.linode.com:5000/announce_user", post_data={ json=j } })
		return handle
	end,
	completion_cb=function (self, success, json)
		if success then
			self.party_data = { partyself=json.partyself, partylist=json.partylist }
			print("completed1234pppp " .. dump(self.party_data))
			ui.update()
		end
	end,
	start=function (self)
		self.party_data = { partyself={}, partylist={} }
		self.http_helper = http_helper:start(5000 * 1000, self, self.issue_request_cb, self.completion_cb)
	end,
	step=function (self)
		self.http_helper:step()
	end,
}
party_refresher:start()

local userlist_refresher = {
	userlist_data=nil,
	issue_request_cb=function (self)
		local j = core.write_json({ hash=core.sha1(core.settings:get("friend_key")), action="userlist" })
		local handle = httpapi.fetch_async({ url="li1826-68.members.linode.com:5000/announce_user", post_data={ json=j } })
		return handle
	end,
	completion_cb=function (self, success, json)
		if success then
			self.userlist_data = { userhash=json.userhash, userlist=json.userlist }
			print("completed1234 " .. dump(self.userlist_data))
			ui.update()
		end
	end,
	start=function (self)
		self.userlist_data = { userhash=nil, userlist={} }
		self.http_helper = http_helper:start(5000 * 1000, self, self.issue_request_cb, self.completion_cb)
	end,
	step=function (self)
		self.http_helper:step()
	end,
}
userlist_refresher:start()

local party_dialog = {
	dialog=nil,
	create=function (self)
		self.dialog = dialog_create("dlg_party",
						self.dialog_formspec,
						self.dialog_button_handler,
						nil)
		self.dialog.data = nil
	end,
	dialog_formspec=function (dialog_dot_data)
		local retval =
			"size[11.5,4.5,true]" ..
			"field[1,1;3,1;fld_dlg_party_name;;default]" ..
			"button[3.25,3.5;2.5,0.5;btn_dlg_party_ok;" .. fgettext("Accept") .. "]"
		return retval
	end,
	dialog_button_handler=function (self, fields)
		print("party name " .. fields["fld_dlg_party_name"])
		if fields["key_enter"] and fields["key_enter_field"] == "fld_dlg_party_name" then
			print("party name updated")
			return true
		end
		if fields["btn_dlg_party_ok"] then
			self:delete()
			return true
		end
		
		return false
	end,
}

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
					v.completion_cb(res.succeeded, v)
				end
			end
		end
	end,
}
auth_refresher:start()

function friend_core_start_override()
	local function completion_cb(succeeded, auth_handle)
		gamedata.external_event_on_connect_data = ""
		if succeeded then
			assert(type(auth_handle.token) == "string")
			gamedata.external_event_on_connect_data = auth_handle.token
		else
			core.log("external_event_on_connect_data not set")
		end
		core.start()
	end
	auth_refresher:auth_request_for("http://example.com", completion_cb)
end

local function asynctestfunc(param)
	party_refresher:step()
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
		"table[0,0.25;5.1,4.3;userlist;" .. cells .. "]" ..
		"button[0,4.85;5.25,0.5;btn_contentdb;".. fgettext("Enable Discord Integration") .. "]"
	retval = retval ..
		"label[6.05,-0.25;" .. fgettext("Online Party:") .. "]" ..
		"tablecolumn[color;text]" ..
		"table[6,0.25;5.1,4.3;partylist;" .. "#FFFFFF,helloworld1234" .. "]" ..
		"button[6,4.85;5.25,0.5;btn_party;" .. fgettext("Create Party") .. "]"
	return retval
end


local function handle_buttons(tabview, fields, tabname, tabdata)
	if fields["userlist"] ~= nil then
		local event = core.explode_table_event(fields["pkglist"])
		print("selected " .. tostring(event.row))
		return true
	end
	if fields["btn_party"] ~= nil then
		party_dialog:create()
		party_dialog.dialog:set_parent(tabview)
		tabview:hide()
		party_dialog.dialog:show()
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
