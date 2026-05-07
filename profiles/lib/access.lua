local ipairs = ipairs

local Access = {}

function Access.find_access_tag(source,access_tags_hierarchy)
    for i,v in ipairs(access_tags_hierarchy) do
        local tag = source:get_value_by_key(v)
        if tag then
            return tag
        end
    end
    return nil
end

function Access.classify_access(value, profile)
    if profile.access_tag_whitelist[value] then return 3 end
    if profile.restricted_access_tag_list[value] then return 2 end
    if profile.access_tag_blacklist[value] then return 1 end
    return 0
end

function Access.resolve_access(value, profile)
    if not value then return nil end
    if not value:find(';') then return value end

    local best = nil
    local best_priority = -1

    for part in value:gmatch('[^;]+') do
        local trimmed = part:match('^%s*(.-)%s*$')
        local priority = Access.classify_access(trimmed, profile)
        if priority > best_priority then
            best = trimmed
            best_priority = priority
        end
    end

    return best or value
end

return Access
