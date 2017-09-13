-- Profile functions dealing with various aspects of relation parsing
--
-- You can run a selection you find useful in your profile,
-- or do you own processing if/when required.

Utils = require('lib/utils')

Relations = {}

function Relations.Merge(relations)
  local result = {}

  for _, r in ipairs(relations) do
    for k, v in pairs(r) do
      result[k] = v
    end
  end

  return result
end

-- match ref values to relations data
function Relations.MatchToRef(relations, ref)

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

  for _, r in ipairs(references) do
    result_match[r] = nil
  end

  for _, rel in ipairs(relations) do
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

  return result_match
end

return Relations