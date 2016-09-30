-- Check run environment for variable value, otherwise return default

function set_option(optionName, default)
    if os.getenv(optionName) == "" then
        return default
    else
        return os.getenv(optionName)
    end
end

return set_option
