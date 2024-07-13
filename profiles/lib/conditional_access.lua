-- Handle conditional access tags as described in the OSM wiki:
-- https://wiki.openstreetmap.org/wiki/Conditional_restrictions

-- Note that we only handle conditional tags for a date range,
-- meant to be used for temporary restrictions, typically due to
-- construction. We also require the date range to be at least a
-- week long



ConditionalAccess = {}


local function parse_conditional_access(way, key)
    local conditional = way:get_value_by_key(key .. ':conditional')
    if not conditional then
      return nil
    end

    -- Examples of conditional tags:
    -- "no @ (2018 May 22-2018 Oct 7)" or
    -- "no @ 2018 Jun 01 - 2018 Jul 23"
    local condition, time_range = conditional:match("([^@]+)@(.+)")
    if not condition or not time_range then
      return nil
    end

    local start_date_str, end_date_str = time_range:match("([^-]+)-(.+)")
    if not start_date_str or not end_date_str then
      return nil
    end

    local function parse_date(date_str)
      local year, month, day = date_str:match("(%d+)%s+(%a+)%s+(%d+)")

      local month_names = {
        Jan = 1, Feb = 2, Mar = 3, Apr = 4, May = 5, Jun = 6,
        Jul = 7, Aug = 8, Sep = 9, Oct = 10, Nov = 11, Dec = 12
      }
      month = month_names[month]
      if not year or not month or not day then
        return nil
      end

      local numericYear = tonumber(year)
      local numericDay = tonumber(day)
      if numericYear and numericDay then
          return os.time({ year = numericYear, month = month, day = numericDay })
      else
          return nil
      end
    end

    local start_date = parse_date(start_date_str)
    local end_date = parse_date(end_date_str)
    local current_date = os.time()

    -- Require start and end date to be more than a week apart
    if not start_date or not end_date or end_date - start_date < 60 * 60 * 24 * 7 then
      return nil
    end

    if current_date >= start_date and current_date <= end_date then
      return condition:match("%S+")
    else
      return nil
    end
  end

function ConditionalAccess.parse_by_set(way, keys)
    for i, key in ipairs(keys) do
        local conditional = parse_conditional_access(way, key)
        if conditional then
            return conditional
        end
    end
    return nil
end

return ConditionalAccess
