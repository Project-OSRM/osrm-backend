-- Profile functions dealing with various aspects of relation parsing
--
-- You can run a selection you find useful in your profile,
-- or do you own processing if/when required.

Utils = require('lib/utils')

Relations = {}

function is_direction(role)
  return (role == 'north' or role == 'south' or role == 'west' or role == 'east')
end

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
    result_match[r] = { forward = nil, backward = nil }
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
        local result_direction = result_match[best_ref]

        local is_forward = rel["route_forward"]
        if is_forward == nil then
          result_direction.forward = direction
          result_direction.backward = direction
        elseif is_forward == true then
          result_direction.forward = direction
        else
          result_direction.backward = direction
        end

        result_match[best_ref] = result_direction
      end
    end

  end

  local result = {}
  for i, r in ipairs(order) do
    result[i] = { ref = r, dir = result_match[r] };
  end

  return result
end

function get_direction_from_superrel(rel, relations)
  local result = nil
  local result_id = nil
  local rel_id_list = relations:get_relations(rel)

  function set_result(direction, current_rel)
    if (result ~= nil) and (direction ~= nil) then
      print('WARNING: relation ' .. rel:id() .. ' is a part of more then one supperrelations ' .. result_id .. ' and ' .. current_rel:id())
      result = nil
    else
      result = direction
      result_id = current_rel:id()
    end
  end

  for i, rel_id in ipairs(rel_id_list) do
    local parent_rel = relations:relation(rel_id)
    if parent_rel:get_value_by_key('type') == 'route' then
      local role = parent_rel:get_role(rel)

      if is_direction(role) then
        set_result(role, parent_rel)
      else
        local dir = parent_rel:get_value_by_key('direction')
        if is_direction(dir) then
          set_result(dir, parent_rel)
        end
      end
    end
    -- TODO: support forward/backward
  end

  return result
end

function Relations.parse_route_relation(rel, way, relations)
  local t = rel:get_value_by_key("type")
  local role = rel:get_role(way)
  local result = {}

  function add_extra_data(m)
    local name = rel:get_value_by_key("name")
    if name then
      result['route_name'] = name
    end

    local ref = rel:get_value_by_key("ref")
    if ref then
      result['route_ref'] = ref
    end
  end

  if t == 'route' then
    local role_direction = nil
    local route = rel:get_value_by_key("route")
    if route == 'road' then
      -- process case, where directions set as role
      if is_direction(role) then
        role_direction = role
      end
    end

    local tag_direction = nil
    local direction = rel:get_value_by_key('direction')
    if direction then
      direction = string.lower(direction)
      if is_direction(direction) then
        tag_direction = direction
      end
    end

    -- determine direction
    local result_direction = role_direction
    if result_direction == nil and tag_direction ~= '' then
      result_direction = tag_direction
    end

    if role_direction ~= nil and tag_direction ~= nil and role_direction ~= tag_direction then
      result_direction = nil
      print('WARNING: conflict direction in role of way ' .. way:id() .. ' and direction tag in relation ' .. rel:id())
    end


    -- process superrelations
    local super_dir = get_direction_from_superrel(rel, relations)

    -- check if there are data error
    if (result_direction ~= nil) and (super_dir ~= nil) and (result_direction ~= super_dir) then
      print('ERROR: conflicting relation directions found for way ' .. way:id() .. 
            ' relation direction is ' .. result_direction .. ' superrelation direction is ' .. super_dir)
      result_direction = nil
    elseif result_direction == nil then
      result_direction = super_dir
    end

    result['route_direction'] = result_direction

    if role == 'forward' then
      result['route_forward'] = true
    elseif role == 'backward' then
      result['route_forward'] = false
    else
      result['route_forward'] = nil
    end

    add_extra_data(m)
  end

  return result
end

function Relations.process_way_refs(way, relations, result)
  local parsed_rel_list = {}
  local rel_id_list = relations:get_relations(way)
  for i, rel_id in ipairs(rel_id_list) do
    local rel = relations:relation(rel_id)
    parsed_rel_list[i] = Relations.parse_route_relation(rel, way, relations)
  end

  -- now process relations data
  local matched_refs = nil;
  if result.ref then
    local match_res = Relations.match_to_ref(parsed_rel_list, result.ref)

    function gen_ref(is_forward)
      local ref = ''
      for _, m in pairs(match_res) do
        if ref ~= '' then
          ref = ref .. '; '
        end

        local dir = m.dir.forward
        if is_forward == false then
          dir = m.dir.backward
        end

        if dir then
          ref = ref .. m.ref .. ' $' .. dir
        else
          ref = ref .. m.ref
        end
      end

      return ref
    end

    result.forward_ref = gen_ref(true)
    result.backward_ref = gen_ref(false)
  end
end

return Relations
