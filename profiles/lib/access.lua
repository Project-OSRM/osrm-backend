local ipairs = ipairs

module "Access"

function find_access_tag(source,access_tags_hierachy)
    for i,v in ipairs(access_tags_hierachy) do
        if source.tags:Find(v) ~= '' then
            return tag
        end
    end
    return nil
end