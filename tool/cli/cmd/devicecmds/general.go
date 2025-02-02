package devicecmds

import (
	"github.com/spf13/cobra"
)

var DeviceCmd = &cobra.Command{
	Use:   "device",
	Short: "Device commands for PoseidonOS.",
	Long: `Device commands for PoseidonOS.

Syntax: 
  poseidonos-cli device [scan|list|smart]

Example (to scan devices in the system):
  poseidonos-cli device scan
	  `,
	Args: cobra.MinimumNArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
	},
}

func init() {
	DeviceCmd.AddCommand(CreateDeviceCmd)
	DeviceCmd.AddCommand(ScanDeviceCmd)
	DeviceCmd.AddCommand(ListDeviceCmd)
	DeviceCmd.AddCommand(SMARTCmd)
}
