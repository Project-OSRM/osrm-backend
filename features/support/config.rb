def profile
  @profile ||= reset_profile
end

def reset_profile
  @profile = nil
  set_profile DEFAULT_SPEEDPROFILE
end

def set_profile profile
  @profile = profile
end

def set_extract_args args
    @extract_args = args
end

def set_contract_args args
    @contract_args = args
end
