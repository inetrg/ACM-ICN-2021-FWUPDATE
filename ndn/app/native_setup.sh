declare -A NODES=(
    [tap0]="BE:63:A1:23:DF:98 /D-A 1"
    [tap1]="92:32:29:2B:04:B8 /D-A 0"
    [tap2]="82:B1:CF:D9:DC:A2 /D-A 0"
    [tap3]="62:18:E6:A1:F2:24 /D-A 0"
    [tap4]="92:BE:38:F4:70:CC /D-A 0"
)

declare -a ROUTES=(
    "tap4 tap0 tap2 down"

    "tap3 tap0 tap2 down"

    "tap2 tap0 tap1 down"

    "tap1 tap0 tap0 down"
)
