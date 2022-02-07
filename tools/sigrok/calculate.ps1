# Assemble array of code segments for storing results
$segmentNamesInterrupts = @("TRIGPRI","TRIGSEC","TRIGTER","ONEMS")
$segmentNamesNonInterrupts = @("INIT","LOOP_start","LOOP_looptimers","LOOP_idlefueladvance","LOOP_mainloop_fuelcalcs","LOOP_mainloop_injectiontiming","LOOP_mainloop_igncalcs","LOOP_mainloop_fuelschedules","LOOP_mainloop_ignschedules","LOOP_mainloop_other","PS_pwfunction","PS_correctionsFuel","PS_docrankspeedcalcs","PS_getCrankAngle","PS_angleToTime","PS_timeToAngle")
$segmentStart = "LOOP_start"
$signalToSegment = $segmentNamesInterrupts + $segmentNamesNonInterrupts;

# Read input data
& "C:\Program Files\sigrok\sigrok-cli\sigrok-cli.exe" -d fx2lafw --config samplerate=2m --samples=1m -P parallel:clock_edge=either:d0=D0:d1=D1:d2=D2:d3=D3:d4=D4:d5=D5:d6=D6:clk=D7 --protocol-decoder-samplenum > sigrok_output.txt
$sigrok_output = Get-Content sigrok_output.txt
[System.Collections.ArrayList]$framesignals = $sigrok_output -replace '^(\d+)-\d+ parallel-\d+: (.*)$','$1,$2' | % { $split = $_ -split ','; [pscustomobject] @{frame = [int32] $split[0]; signal=[Int16] ("0x"+$split[1])} }

# Make sure we start with a specific "root" segment
while ($framesignals.count -gt 1) {
    if ($framesignals[0].signal -eq $framesignals[1].signal -and $framesignals[0].signal -eq $signalToSegment.IndexOf($segmentStart) ) { break }
    else { $framesignals.RemoveAt(0) }
}
# Make sure we end with a specific "root" segment
while ($framesignals.count -gt 1) {
    if ($framesignals[$framesignals.count-1].signal -eq $framesignals[$framesignals.count-2].signal -and $framesignals[$framesignals.count-1].signal -eq $signalToSegment.IndexOf($segmentStart) ) { break }
    else { $framesignals.RemoveAt($framesignals.count-1) }
}

if ($framesignals.count -lt 2) {
    Write-error "Not enough data"
    exit
}

Write-Output "Using $($framesignals.count) samples"

# For total duration percentage calcalations
$startFrame = $framesignals[0].Frame
$endFrame = 0
$rootOtherStartFrame = 0;

# cb = code block
$cbCurrent = -1
[System.Collections.ArrayList]$cb = @()

# Output table
[System.Collections.ArrayList]$segments = @()
[void]$segments.Add( [pscustomobject] @{ name="other"; duration=0; childrenduration=0; children = [System.Collections.ArrayList] @(); parent=$null; count=0 } )
[System.Collections.ArrayList]$segmentsOut = @()

foreach($fs in $framesignals)
{
    if ($fs.signal -ge $signalToSegment.Count) { Write-Error "Bad input data"; exit }
    $segmentName = $signalToSegment[$fs.signal]

    if ( $cbCurrent -gt -1 -and $segmentName -eq $cb[$cbCurrent].name) { # We are ending an open codeblock
        
        # Duration of this codeblock
        $cbDuration = $fs.Frame - $cb[$cbCurrent].startFrame - $cb[$cbCurrent].interruptDuration

        # Add to the segment connected to this codeblock
        $cb[$cbCurrent].segment.count++
        $cb[$cbCurrent].segment.duration += $cbDuration
        $cb[$cbCurrent].segment.childrenDuration += $cb[$cbCurrent].childrenDuration

        # Add non children time to other
        $cb[$cbCurrent].segment.children[0].duration += $cbDuration - $cb[$cbCurrent].childrenDuration
        $cb[$cbCurrent].segment.children[0].count++

        # If this segment has a parent, add to it
        if ( $cb[$cbCurrent].segment.parent -ne $null ) {
            $cb[$cbCurrent-1].childrenDuration += $cbDuration
            $cb[$cbCurrent-1].interruptDuration += $cb[$cbCurrent].interruptDuration
        }
        # If it doesn't it might be an interrupt which we need to take into account
        elseif ( $cb[$cbCurrent].segment.parent -eq $null -and $cbCurrent -gt -0 ) {
            $cb[$cbCurrent-1].interruptDuration += $cbDuration
        }

        # Remove this handled codeblock
        $cb.RemoveAt($cbCurrent)
        $cbCurrent--

        # Only write output after root function has been found. This prevent children getting counted when parent isn't at end of file
        if ($cbCurrent -eq -1) {
            # "Deep copy" meaning every object and reference gets duplicated
            $segmentsOut = [Management.Automation.PSSerializer]::DeSerialize([Management.Automation.PSSerializer]::Serialize($segments))
            # Capture the frame (time) of the last closed segment for total duration calculation
            $endFrame = $fs.Frame

            $rootOtherStartFrame = $fs.Frame
        }
    }
    else { # We are in a new code block
        
        if ($rootOtherStartFrame -gt 0) {
            $segments[0].duration += $fs.frame - $rootOtherStartFrame
            $segments[0].count++
            $rootOtherStartFrame = 0;
        }

        # Add the new code block
        $cb += [pscustomobject] @{ name=$segmentName; startFrame=$fs.Frame; childrenDuration=0; interruptDuration=0; segment=$null }
        $cbCurrent++;

        # Find in which list to check-or-add the code block
        [System.Collections.ArrayList]$segmentList = @()
        $parent = $null
        if ($fs.Signal -lt $segmentNamesInterrupts.Count -or $cbCurrent -eq 0) { # Interrupt or "root" segment
            $segmentList = $segments
        }
        else { # All "non root" segments
            $parent = $cb[$cbCurrent-1].segment
            $segmentList = $parent.children
        }

        # Make sure segment is in the list
        if ( $segmentList.name -notcontains $segmentName ) {
            $added = $segmentList.Add( [pscustomobject] @{ name=$segmentName; duration=0; childrenduration=0; children = [System.Collections.ArrayList] @(); parent=$parent; count=0 } )
            [void]$segmentList[$added].children.Add( [pscustomobject] @{ name="other"; duration=0; childrenduration=0; children = [System.Collections.ArrayList] @(); parent=$segmentList[$added]; count=0 } )
        }

        # Store a reference to the target segment
        $cb[$cbCurrent].segment = $segmentList | Where-Object Name -eq $segmentName

    }

}


# Prepare the result
$totalDuration = $endFrame - $startFrame

[System.Collections.ArrayList]$outputList = @()

function parseList {
    param ( $segmentsIn, [int16]$hierarchyLevel )

    $hierarchyLevel++

    $segmentsDuration = $segmentsIn | Measure-Object -sum duration | Select-Object Sum

    $segmentsIn = $segmentsIn | select-object name,duration,@{Name = 'PercentageOfTotalDecimal';Expression={($_.duration/$totalDuration)}},childrenduration,@{Name = 'PercentageOfSegmentDecimal';Expression={(($_.duration)/$segmentsDuration.Sum)}},children,count

    $segmentsIn = $segmentsIn | Sort-Object -Property PercentageOfTotalDecimal -Descending

    Foreach ($segmentIn in $segmentsIn) {
        $output = $segmentIn
        $output = $output | Select-object -Property *,@{Name = 'PercentageOfTotal';Expression={" " * 2 * $hierarchyLevel + ($_.PercentageOfTotalDecimal.ToString("P"))}}
        $output = $output | Select-object -Property *,@{Name = 'PercentageOfSegment';Expression={" " * 2 * $hierarchyLevel + ($_.PercentageOfSegmentDecimal.ToString("P"))}}
        $output = $output | Select-object -Property name,PercentageOfTotal,PercentageOfSegment,count
        [void]$outputList.Add( $output )

        if ($segmentIn.children.Count -gt 1) {
            $segmentIn.children | ForEach { $_.name = " " * 2 * $hierarchyLevel + $_.name }
            parseList -segments $segmentIn.children -hierarchyLevel $hierarchyLevel
        }
    }
}

parseList -segments $segmentsOut -hierarchyLevel 0

$outputList | Format-Table

#$totalDuration
#$segmentsOut | Measure-Object -sum duration | Select-Object Sum

# PercentageOfTotal and PercentageOfSegment does not match, there is an error somewhere