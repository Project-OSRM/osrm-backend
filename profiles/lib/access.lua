local ipairs = ipairs

module "Access"

function find_access_tag(source, access_tags_hierachy)
  for i,v in ipairs(access_tags_hierachy) do
    local access_tag = source:get_value_by_key(v, "")
    if "" ~= access_tag then
      return access_tag
    end
  end
  return ""
end
