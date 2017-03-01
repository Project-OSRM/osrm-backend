set(GITHUB_API_TOKEN $ENV{GITHUB_API_TOKEN})

set(GITHUB_AUTH "")
if(GITHUB_API_TOKEN)
    set(GITHUB_AUTH "?access_token=${GITHUB_API_TOKEN}")
endif()

set(GITHUB_API_URL "https://api.github.com")

function(github_get_rate_limit VAR)
    set(RATE_LIMIT_FILE ${CMAKE_CURRENT_BINARY_DIR}/GITHUBRATE)
    set(RATE_LIMIT_URL ${GITHUB_API_URL}/rate_limit${GITHUB_AUTH})
    nodejs_download(
        ${RATE_LIMIT_URL}
        ${RATE_LIMIT_FILE}
        ON
    )
    file(READ ${RATE_LIMIT_FILE} RATE_LIMIT_DATA)
    string(REGEX MATCH "\"remaining\": ([0-9]+),"
        RATE_LIMIT_MATCH ${RATE_LIMIT_DATA})
    set(${VAR} ${CMAKE_MATCH_1} PARENT_SCOPE)
endfunction()

mark_as_advanced(
    GITHUB_AUTH
    GITHUB_API_TOKEN
    GITHUB_API_URL
)