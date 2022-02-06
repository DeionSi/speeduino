# Assemble array of code segments for storing results
$segmentNamesInterrupts = @("TRIGPRI","TRIGSEC","TRIGTER","ONEMS")
$segmentNamesNonInterrupts = @("INIT","LOOP_start","LOOP_looptimers","LOOP_idlefueladvance","LOOP_mainloop_fuelcalcs","LOOP_mainloop_injectiontiming","LOOP_mainloop_igncalcs","LOOP_mainloop_fuelschedules","LOOP_mainloop_ignschedules","LOOP_mainloop_other","PS_pwfunction","PS_correctionsFuel","PS_docrankspeedcalcs","PS_getCrankAngle")
$segmentStart = "LOOP_start"
$signalToSegment = $segmentNamesInterrupts + $segmentNamesNonInterrupts;

# Read input data
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

# cb = code block
$cbCurrent = -1
[System.Collections.ArrayList]$cb = @()

# Output table
[System.Collections.ArrayList]$segments = @()
[System.Collections.ArrayList]$segmentsOut = @()

foreach($fs in $framesignals)
{
    if ($fs.signal -ge $signalToSegment.Count) { Write-Error "Bad input data"; exit }
    $segmentName = $signalToSegment[$fs.signal]
    

    if ( $cbCurrent -gt -1 -and $segmentName -eq $cb[$cbCurrent].name) { # We are ending an open codeblock
        
        # Duration of this codeblock
        $cbDuration = $fs.Frame - $cb[$cbCurrent].startFrame

        # Add to the segment connected to this codeblock
        $cb[$cbCurrent].segment.count += 1
        $cb[$cbCurrent].segment.duration += $cbDuration - $cb[$cbCurrent].interruptDuration
        $cb[$cbCurrent].segment.childrenDuration += $cb[$cbCurrent].childrenDuration

        # If this segment has a parent, add to it
        if ( $cb[$cbCurrent].segment.parent -ne $null ) {
            $cb[$cbCurrent-1].childrenDuration += $cbDuration - $cb[$cbCurrent].interruptDuration
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
        }
    }
    else { # We are in a new code block
        
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
            [void]$segmentList.Add( [pscustomobject] @{ name=$segmentName; duration=0; childrenduration=0; children = [System.Collections.ArrayList] @(); parent=$parent; count=0 } )
        }

        # Store a reference to the target segment
        $cb[$cbCurrent].segment = $segmentList | Where-Object Name -eq $segmentName

    }

}


# Prepare the result
$totalDuration = $endFrame - $startFrame
$totalDurationInProfiling = 0

[System.Collections.ArrayList]$outputList = @()

function parseList {
    param ( $segmentsIn, [int16]$hierarchyLevel )

    $hierarchyLevel++

    $segmentsIn = $segmentsIn | select-object name,duration,@{Name = 'PercentageDecimalTotal';Expression={($_.duration/$totalDuration)}},childrenduration,@{Name = 'PercentageDecimalExclChildren';Expression={(($_.duration-$_.childrenduration)/$totalDuration)}},children,count

    $segmentsIn = $segmentsIn | Sort-Object -Property PercentageDecimalTotal -Descending

    Foreach ($segmentIn in $segmentsIn) {
        $Global:totalDurationInProfiling += $segmentIn.duration - $segmentIn.childrenduration
        $output = $segmentIn
        $output = $output | Select-object -Property *,@{Name = 'PercentageTotal';Expression={" " * 2 * $hierarchyLevel + ($_.PercentageDecimalTotal.ToString("P"))}}
        $output = $output | Select-object -Property *,@{Name = 'PercentageExclChildren';Expression={" " * 2 * $hierarchyLevel + ($_.PercentageDecimalExclChildren.ToString("P"))}}
        $output = $output | Select-object -Property name,PercentageTotal,PercentageExclChildren,count
        [void]$outputList.Add( $output )

        $segmentIn.children | ForEach { $_.name = " " * 2 * $hierarchyLevel + $_.name }

        parseList -segments $segmentIn.children -hierarchyLevel $hierarchyLevel
    }
}

parseList -segments $segmentsOut -hierarchyLevel 0

$outputList | Format-Table

$totalDurationInProfiling
$totalDuration