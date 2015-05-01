local ipairs = ipairs

local Access = {}

function Access.find_access_tag(source,access_tags_hierachy)
    for i,v in ipairs(access_tags_hierachy) do
        local tag = source:get_value_by_key(v)
        if tag and tag ~= '' then
            return tag
        end
    end
    return ""
end

return Access
