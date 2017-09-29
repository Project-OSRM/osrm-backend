-- Profile functions dealing with various aspects of relation parsing
--
-- You can run a selection you find useful in your profile,
-- or do you own processing if/when required.

Utils = require('lib/utils')

Relations = {}

-- match ref values to relations data
function Relations.match_to_ref(relations, ref)

  function calculate_scores(refs, tag_value)
    local tag_tokens = Set(Utils.tokenize_common(tag_value))
    local result = {}
    for i, r in ipairs(refs) do
      local ref_tokens = Utils.tokenize_common(r)
      local score = 0

      for _, t in ipairs(ref_tokens) do
        if tag_tokens[t] then
          if Utils.is_number(t) then
            score = score + 2
          else
            score = score + 1
          end
        end
      end

      result[r] = score
    end

    return result
  end

  local references = Utils.string_list_tokens(ref)
  local result_match = {}
  local order = {}
  for i, r in ipairs(references) do
    result_match[r] = false
    order[i] = r
  end

  for i, rel in ipairs(relations) do
    local name_scores = nil
    local name_tokens = {}
    local route_name = rel["route_name"]
    if route_name then
      name_scores = calculate_scores(references, route_name)
    end

    local ref_scores = nil
    local ref_tokens = {}
    local route_ref = rel["route_ref"]
    if route_ref then
      ref_scores = calculate_scores(references, route_ref)
    end

    -- merge scores
    local direction = rel["route_direction"]
    if direction then
      local best_score = -1
      local best_ref = nil
      
      function find_best(scores)
        if scores then
          for k ,v in pairs(scores) do
            if v > best_score then
              best_ref = k
              best_score = v
            end
          end
        end
      end

      find_best(name_scores)
      find_best(ref_scores)
      
      if best_ref then
        result_match[best_ref] = direction
      end
    end

  end

  local result = {}
  for i, r in ipairs(order) do
    result[i] = { ref = r, dir = result_match[r] };
  end

  return result
end

function Relations.parse_route_relation(relation, obj)
  local t = relation:get_value_by_key("type")
  local role = relation:get_role(obj)
  local result = {}

  function add_extra_data(m)
    local name = relation:get_value_by_key("name")
    if name then
      result['route_name'] = name
    end

    local ref = relation:get_value_by_key("ref")
    if ref then
      result['route_ref'] = ref
    end
  end

  if t == 'route' then
    local route = relation:get_value_by_key("route")
    if route == 'road' then
      -- process case, where directions set as role
      if role == 'north' or role == 'south' or role == 'west' or role == 'east' then
        result['route_direction'] = role
        add_extra_data(m)
      end
    end

    local direction = relation:get_value_by_key('direction')
    if direction then
      direction = string.lower(direction)
      if direction == 'north' or direction == 'south' or direction == 'west' or direction == 'east' then
        if role == 'forward' then
          result['route_direction'] = direction
          add_extra_data(m)
        end
      end
    end
  end

  return result
end

return Relations